
#include "../../../core/common.h"
#include "srs_librtmp.h"
#include "srs_kernel_codec.h"

static int plugin_open_rtmp(source_layer*pl, player_parameter *pp,FILLSAMPLE fillsample, void *key);
static int plugin_set_notify_rtmp(source_layer*pl,MSG_GATEWAY notify, void *key );
static int plugin_start_rtmp(source_layer*pl,int mspos);
static int plugin_eof_rtmp(source_layer*pl);
static int plugin_close_rtmp(source_layer*pl);
static int plugin_seek_rtmp(source_layer*pl,int ms);
static int plugin_pause_rtmp(source_layer*pl);

enum RTMPLAYER_STATUS
{
	RS_PLY_Init,
	RS_PLY_Handshaked,
	RS_PLY_Connected,
	RS_PLY_Played,
	RS_PLY_Closed
};

#define URL_SIZE		2000
#define MAX_RETRY_TIME  3

#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS                       0
#endif

extern int aac_sample_rates[];

typedef struct DemuxData
{
	DemuxData(int size) : _data(NULL), _data_len(0), _data_size(size){
		_data = new char[_data_size];
	}
	virtual ~DemuxData(void){ delete[] _data; }
	void reset() {
		_data_len = 0;
	}
	int append(const char* pData, int len)
	{
		if (_data_len + len > _data_size){
			int new_size = _data_len + len + 1024 * 2;
			char *pOld = _data;
			_data = new char[new_size];
			if( !_data ){
				_data = pOld;
				return 0;
			}
			memcpy( _data, pOld, _data_len );
			_data_size = new_size;
			delete []pOld;
		}
		memcpy(_data + _data_len, pData, len);
		_data_len += len;
		return len;
	}

	char*_data;
	int _data_len;
	int _data_size;
}DemuxData;

typedef struct srs_rtmp_client
{
	/* rtmp handle */
	void*				rtmp_;
	SrsAvcAacCodec*		srs_codec_;
	int					retry_ct_;
	/*rtmp URI */
	char				uri[URL_SIZE];

	/* pause the stream */
	int pause;

	/* end of stream*/
	int eof;
	RTMPLAYER_STATUS	rtmp_status_;

	DemuxData*			audio_payload_;
	DemuxData*			video_payload_;

	/*worker thread */
	int					running;
	THREAD_HANDLE_TYPE	worker_handle;
	THREAD_ID_TYPE		pb_thread_id;

	/*notify callback*/
	MSG_GATEWAY notify_routine;
	void *key;

	/*data push callback*/
	FILLSAMPLE	fillsample;
	void		*fillkey;

	int			param_present[3];

	void		*locker;

	int			recv_data;

	TIME_TYPE	previous_time[MAX_STREAM_NUM];
	TIME_TYPE	offset_time[MAX_STREAM_NUM];

}srs_rtmp_client;

static u_int8_t fresh_nalu_header[] = { 0x00, 0x00, 0x00, 0x01 };

static int rtmp_Available( const long parm )
{
	const char *url = (const char*)parm;
	if( !url ) return 0;
	// only support rtmp
	if( !strncmp(url, "rtmp://", 7) )
	{
		return 1;
	}
	return 0;
}

static void* rtmp_CreateModule(const void * parm, int size )
{
	source_layer *pl = (source_layer *)vpc_mem_alloc(sizeof(source_layer));
	if( pl ) 
	{
		pl->plug_pri_data = vpc_mem_alloc( sizeof(srs_rtmp_client));
		if( !pl->plug_pri_data ) 
		{
			vpc_mem_free(pl);
			return 0;
		}
		pl->open		= plugin_open_rtmp;
		pl->set_notify	= plugin_set_notify_rtmp;
		pl->start		= plugin_start_rtmp;
		pl->close		= plugin_close_rtmp;
		pl->seek		= plugin_seek_rtmp;
		pl->pause		= plugin_pause_rtmp;
		pl->eof			= plugin_eof_rtmp;
	}
	return pl;
}

static void rtmp_ReleaseModule(void *p)
{
	vpc_mem_free(p);
}

static const node_boot_func srs_librtmp_demuxer = {
	"rtmp demuxer",
	"srs libRTMP demux",
	SOURCE_ID,
	RTMP_ID,
	PRIORITY_STANDARD,
	0,
	rtmp_Available,
	rtmp_CreateModule,
	rtmp_ReleaseModule
};

void srs_rtmp_stream_register(void *ptx)
{
	RegisterTools(ptx,&srs_librtmp_demuxer,0);
}


static RTMPLAYER_STATUS CallDisconnect(srs_rtmp_client *p, RTMPLAYER_STATUS cur_status)
{
	RTMPLAYER_STATUS ret = RS_PLY_Init;
	mutex_lock(p->locker);
	if (p->rtmp_) {
		srs_rtmp_destroy(p->rtmp_);
		p->rtmp_ = NULL;
	}
	if(cur_status != RS_PLY_Closed) {
		p->retry_ct_ ++;
		if(p->retry_ct_ <= MAX_RETRY_TIME)
		{
			p->rtmp_ = srs_rtmp_create(p->uri);

		} else {
			ret = RS_PLY_Closed;
			p->notify_routine( p->key, MSG_BY_PASS, VPC_NET_TIME_OUT, 0);
			vpc_delay(30);
		}
	}
	mutex_unlock(p->locker);
	return ret;
}

static void CallConnect(srs_rtmp_client *p)
{
	p->retry_ct_ = 0;
}

static void RescanVideoframe(srs_rtmp_client *pclient,const char*pdata, int len, uint32_t timestamp)
{
	int nal_type = pdata[4] & 0x1f;
	const char *p = pdata;
	if (nal_type == 7)
	{// keyframe
		int find7 = 0;
		const char* ptr7 = NULL;
		int size7 = 0;
		int find8 = 0;
		const char* ptr8 = NULL;
		int size8 = 0;
		const char* ptr5 = NULL;
		int size5 = 0;
		int head01 = 4;
		for (int i = 4; i < len - 4; i++)
		{
			if ((p[i] == 0x0 && p[i + 1] == 0x0 && p[i + 2] == 0x0 && p[i + 3] == 0x1) || (p[i] == 0x0 && p[i + 1] == 0x0 && p[i + 2] == 0x1))
			{
				if (p[i + 2] == 0x01)
					head01 = 3;
				else
					head01 = 4;
				if (find7 == 0)
				{
					find7 = i;
					ptr7 = p;
					size7 = find7;
					i++;
				}
				else if (find8 == 0)
				{
					find8 = i;
					ptr8 = p + find7 ;
					size8 = find8 - find7;
					const char* ptr = p + i;
					if ((ptr[head01] & 0x1f) == 5)
					{
						ptr5 = p + find8 + head01;
						size5 = len - find8 - head01;
						break;
					}
				}
				else
				{
					ptr5 = p + i + head01;
					size5 = len - i - head01;
					break;
				}
			}
		}
		pclient->video_payload_->append(ptr7, size7);
		pclient->video_payload_->append(ptr8, size8);
		pclient->video_payload_->append((const char*)fresh_nalu_header, 4);
		pclient->video_payload_->append(ptr5, size5);
		//callback_.OnRtmpullH264Data((uint8_t*)video_payload_->_data, video_payload_->_data_len, timestamp);
		pclient->video_payload_->reset();
	}
	else 
	{
		pclient->video_payload_->append(pdata, len);
		//callback_.OnRtmpullH264Data((uint8_t*)video_payload_->_data, video_payload_->_data_len, timestamp);
		pclient->video_payload_->reset();
	}

}

TIME_TYPE compute_media_time(srs_rtmp_client *p, int media, TIME_TYPE dts){
	return dts;
	if( dts < p->previous_time[media] ){
		p->offset_time[media] += p->previous_time[media];

		if( media == TM_MEDIA_AUDIO )
			vpc_printf("audio time round\r\n");
		else
			vpc_printf("video time round\r\n");
	}
	p->previous_time[media] =  dts;
	return dts + p->offset_time[media];
}

static int GotVideoSample(srs_rtmp_client *p, u_int32_t timestamp, SrsCodecSample *sample)
{
	char length[4];
	int t;
	int ret = ERROR_SUCCESS;
/*	static FILE *fp = 0;*/

// 	if(!fp)
// 		fp = fopen("d:\\dd.264","wb");

	// ignore info frame,
	// @see https://github.com/simple-rtmp-server/srs/issues/288#issuecomment-69863909
	if (sample->frame_type == SrsCodecVideoAVCFrameVideoInfoFrame) {
		return ret;
	}

	// ignore sequence header
	if (sample->frame_type == SrsCodecVideoAVCFrameKeyFrame
		&& sample->avc_packet_type == SrsCodecVideoAVCTypeSequenceHeader) {
			return ret;
	}

	// when ts message(samples) contains IDR, insert sps+pps.
	if (sample->has_idr ) {
		// fresh nalu header before sps.
		t = p->srs_codec_->sequenceParameterSetLength;
		if (t > 0) {
			length[0] = (t&0xFF000000)>>24;
			length[1] = (t&0xFF0000)>>16;
			length[2] = (t&0xFF00)>>8;
			length[3] = (t&0xFF);
			p->video_payload_->append((const char*)length, 4);
			// sps
			p->video_payload_->append(p->srs_codec_->sequenceParameterSetNALUnit, t);

		}

		t = p->srs_codec_->pictureParameterSetLength;
		if (t > 0)
		{
			length[0] = (t&0xFF000000)>>24;
			length[1] = (t&0xFF0000)>>16;
			length[2] = (t&0xFF00)>>8;
			length[3] = (t&0xFF);
			p->video_payload_->append((const char*)length, 4);
			// pps
			p->video_payload_->append(p->srs_codec_->pictureParameterSetNALUnit, t);
		}

		if( !p->param_present[TM_MEDIA_VIDEO] )
		{
			MEDIA_FMT media_fmt = {0};
			int cnt = media_fmt.cnt;
			media_fmt.fmt[cnt].nType = TM_MEDIA_VIDEO;
			media_fmt.fmt[cnt].nFourCC = H264_ID;
			strcpy(media_fmt.fmt[cnt].codecname,"H264");
			media_fmt.cnt++;
			p->param_present[TM_MEDIA_VIDEO] = 1;
			p->notify_routine( p->key, MSG_MEDIA_FMT, (long)&media_fmt, sizeof(media_fmt));
			vpc_printf("reach video spec\r\n");;
		}
	}

	// all sample use cont nalu header, except the sps-pps before IDR frame.
	for (int i = 0; i < sample->nb_sample_units; i++) {
		SrsCodecSampleUnit* sample_unit = &sample->sample_units[i];
		int32_t size = sample_unit->size;

		if (!sample_unit->bytes || size <= 0) {
			ret = -1;
			return ret;
		}

		// 5bits, 7.3.1 NAL unit syntax,
		// H.264-AVC-ISO_IEC_14496-10-2012.pdf, page 83.
		SrsAvcNaluType nal_unit_type = (SrsAvcNaluType)(sample_unit->bytes[0] & 0x1f);

		// ignore SPS/PPS/AUD
		switch (nal_unit_type) {
		case SrsAvcNaluTypeSPS:
		case SrsAvcNaluTypePPS:
		case SrsAvcNaluTypeSEI:
		case SrsAvcNaluTypeAccessUnitDelimiter:
			continue;
		default: {
			if (nal_unit_type == SrsAvcNaluTypeReserved) {
				RescanVideoframe(p,sample_unit->bytes, sample_unit->size, timestamp);
				continue;
				}
			}
		 break;
		}

		t = sample_unit->size;
		length[0] = (t&0xFF000000)>>24;
		length[1] = (t&0xFF0000)>>16;
		length[2] = (t&0xFF00)>>8;
		length[3] = (t&0xFF);

		// sample data
		p->video_payload_->append((const char*)length, 4);
		p->video_payload_->append(sample_unit->bytes, sample_unit->size);

	}

	if( p->video_payload_->_data_len )
	{
		stream_pack sp = {0};
		sp.dts = timestamp;//compute_media_time(p,TM_MEDIA_VIDEO, timestamp);
		sp.vtag.rt = vpc_gettime();
		sp.style = TM_MEDIA_VIDEO;
		sp.frame_style = '?';

		if( sample->frame_type  ==  SrsCodecVideoAVCFrameKeyFrame ){
			sp.frame_style = 'I';
		}
		else if( sample->frame_type  ==  SrsCodecVideoAVCFrameInterFrame){
			sp.frame_style = 'P';
		}
		else if( sample->frame_type  ==  SrsCodecVideoAVCFrameDisposableInterFrame){
			sp.frame_style = 'B';
		}
		sp.buffer = (uint8_t *)p->video_payload_->_data;
		sp.buf_size = p->video_payload_->_data_len;

// 		fwrite(sp.buffer, sp.buf_size, 1, fp);
// 		fflush(fp);
 
		p->fillsample(p->fillkey,&sp);

		p->video_payload_->reset();
		p->recv_data++;
	}

	return ret;
}

static int GotAudioSample(srs_rtmp_client *p,u_int32_t timestamp, SrsCodecSample *sample)
{
	int size = 0;
	int ret = ERROR_SUCCESS;
	for (int i = 0; i < sample->nb_sample_units; i++) {
		SrsCodecSampleUnit* sample_unit = &sample->sample_units[i];
		int32_t size = sample_unit->size;

		if (!sample_unit->bytes || size <= 0 || size > 0x1fff) {
			ret = -1;
			return ret;
		}

		if ( !p->param_present[TM_MEDIA_AUDIO] ){
			/* media format */
			MEDIA_FMT media_fmt = {0};

			media_fmt.fmt[0].nType = TM_MEDIA_AUDIO;
			media_fmt.fmt[0].nFourCC = AAC_ID;
			strcpy(media_fmt.fmt[0].codecname,"AAC");

			// get audio spec
			media_fmt.fmt[0].av.a.nBitsPerSample = 16;
			media_fmt.fmt[0].av.a.nChannels = p->srs_codec_->aac_channels;
			media_fmt.fmt[0].av.a.nSamplerate = aac_sample_rates[p->srs_codec_->aac_sample_rate];

			//sample:channel:sbr:ps
			size = sprintf(media_fmt.fmt[0].extradata,"%d/%d/%d/%d",aac_sample_rates[p->srs_codec_->aac_sample_rate], p->srs_codec_->aac_channels,0, 0);
			media_fmt.fmt[0].extrasize = size;

			//p->media_fmt.fmt[cnt].extrasize = flvdemux.flv_tag.date_size-2;

			media_fmt.cnt++;
			p->param_present[TM_MEDIA_AUDIO] = 1;
			p->notify_routine( p->key, MSG_MEDIA_FMT, (long)&media_fmt, sizeof(media_fmt));
			vpc_printf("reach audio spec\r\n");;
		}
		size = sample_unit->size;

		char aac_length[2];
		aac_length[0] = (size>>8);
		aac_length[1] = size;

		stream_pack sp = {0};
		sp.frame_style = 'I';
		sp.dts = timestamp;//compute_media_time(p,TM_MEDIA_AUDIO, timestamp);
		sp.vtag.rt = vpc_gettime();
		sp.style = TM_MEDIA_AUDIO;

		// copy to audio buffer
		p->audio_payload_->append((const char*)aac_length, 2);
		p->audio_payload_->append(sample_unit->bytes, sample_unit->size);

		/*fill sample data*/
		sp.buffer	= (uint8_t*)p->audio_payload_->_data;
		sp.buf_size = p->audio_payload_->_data_len;
		p->fillsample(p->fillkey,&sp);
		p->recv_data++;
		p->audio_payload_->reset();

// 		{
// 			static FILE *fp = 0;
// 			if( !fp)
// 				fp = fopen("d://audiots.txt","wb");
// 			if( fp){
// 				fprintf(fp, "%d\r\n", sp.ts);
// 			}
// 		}

	}

	return ret;
}

/* 
	0  reconnect 
	-1 no object
	1  ok
*/

static int DoReadData(srs_rtmp_client *p)
{
	int size;
	int ret;
	char type;
	char* data;
	u_int32_t timestamp;

	if ( (ret = srs_rtmp_read_packet(p->rtmp_, &type, &timestamp, &data, &size) )!= 0 ) {
		if( p->recv_data ){
			vpc_delay(100);
			p->recv_data = 0;
			vpc_printf("srs_rtmp_read_packet return 0.......\r\n");
			return 0; //reconnect
		}
		else if( p->running )
		{
			p->running = 0;
			p->notify_routine( p->key, MSG_BY_PASS, VPC_NO_PLAY_OBJECT, 0);
			return -1;
		}
	}

	if (type == SRS_RTMP_TYPE_VIDEO) {
		SrsCodecSample sample;
		if (p->srs_codec_->video_avc_demux(data, size, &sample) == ERROR_SUCCESS) {
			if (p->srs_codec_->video_codec_id == SrsCodecVideoAVC) {	// Just support H264
				GotVideoSample(p,timestamp, &sample);
			}
			else {
				vpc_printf("Don't support video format!\r\n");
			}
		}
	}
	else if (type == SRS_RTMP_TYPE_AUDIO) {
		SrsCodecSample sample;
		if (p->srs_codec_->audio_aac_demux(data, size, &sample) != ERROR_SUCCESS) {
			if (sample.acodec == SrsCodecAudioMP3 && p->srs_codec_->audio_mp3_demux(data, size, &sample) != ERROR_SUCCESS) {
				free(data);
				return -2;
			}
			free(data);
			return -2;	// Just support AAC.
		}
		SrsCodecAudio acodec = (SrsCodecAudio)p->srs_codec_->audio_codec_id;

		// ts support audio codec: aac/mp3
		if (acodec != SrsCodecAudioAAC && acodec != SrsCodecAudioMP3) {
			free(data);
			return -2;
		}
		// for aac: ignore sequence header
		if (acodec == SrsCodecAudioAAC && sample.aac_packet_type == SrsCodecAudioTypeSequenceHeader 
			|| p->srs_codec_->aac_object == SrsAacObjectTypeReserved) {
				free(data);
				return -2;
		}
		GotAudioSample(p, timestamp, &sample);
	}
	else if (type == SRS_RTMP_TYPE_SCRIPT) {
		if (!srs_rtmp_is_onMetaData(type, data, size)) {
			//vpc_printf("No flv\r\n");
			//srs_human_trace("drop message type=%#x, size=%dB", type, size);
		}
	}
	else {
		vpc_delay(20);
	}
	free(data);
	return 1;
}

THREAD_RETURN_TYPE VPC_API SRS_RtmpStreamThread(  THREAD_PARAM_TYPE lpParameter )
{
	/* source player */
	source_layer *pl = (source_layer*)lpParameter;

	/* */
	srs_rtmp_client *p = (srs_rtmp_client*)pl->plug_pri_data;
	RTMPLAYER_STATUS	rtmp_status_   = RS_PLY_Init;

	p->srs_codec_		= new SrsAvcAacCodec();
	p->audio_payload_	= new DemuxData(512);
	p->video_payload_	= new DemuxData(128 * 1024);

	while( p->running )
	{  
		if (p->rtmp_ != NULL)
		{
			switch (rtmp_status_) {
			case RS_PLY_Init:
				{
					if (srs_rtmp_handshake(p->rtmp_) == 0) {
						srs_human_trace("SRS: simple handshake ok.");
						rtmp_status_ = RS_PLY_Handshaked;
					}
					else {
						rtmp_status_ = CallDisconnect(p, RS_PLY_Handshaked);
					}
				}
				break;
			case RS_PLY_Handshaked:
				{
					if (srs_rtmp_connect_app(p->rtmp_) == 0) {
						srs_human_trace("SRS: connect vhost/app ok.");
						rtmp_status_ = RS_PLY_Connected;
					}
					else {
						rtmp_status_ = CallDisconnect(p, RS_PLY_Handshaked );
					}
				}
				break;
			case RS_PLY_Connected:
				{
					if (srs_rtmp_play_stream(p->rtmp_) == 0) {
						srs_human_trace("SRS: play stream ok.");
						rtmp_status_ = RS_PLY_Played;
						CallConnect(p);
					}
					else {
						rtmp_status_ = CallDisconnect(p, RS_PLY_Connected );
					}
				}
				break;
			case RS_PLY_Played:
				{
					if( DoReadData(p) == 0 ){
						//reconnect
						rtmp_status_ =  CallDisconnect(p, RS_PLY_Played);
					}
				}
				break;
			}
		}
		else{
			p->running = 0;
		}
	}

	if (p->srs_codec_) {
		delete p->srs_codec_;
		p->srs_codec_ = NULL;
	}
	if (p->audio_payload_) {
		delete p->audio_payload_;
		p->audio_payload_ = NULL;
	}
	if (p->video_payload_) {
		delete p->video_payload_;
		p->video_payload_ = NULL;
	}
	vpc_printf("rtmp thread finish\r\n");
	return 0;
}

//--------------------------plugin interface----------------------endif

int plugin_open_rtmp(source_layer*pl, player_parameter *pp,FILLSAMPLE fillsample, void *key)
{
	srs_rtmp_client *s = (srs_rtmp_client *)pl->plug_pri_data;
	if( !s ) return 0;
	s->fillsample = fillsample;
	s->fillkey	= key;
	strcpy(s->uri, pp->play_url);
	return 1;
}

int plugin_set_notify_rtmp(source_layer*pl,MSG_GATEWAY notify, void *key )
{
	srs_rtmp_client *s = (srs_rtmp_client *)pl->plug_pri_data;
	if( !s ) return 0;
	s->notify_routine = notify;
	s->key = key;
	return 1;
}

int plugin_start_rtmp(source_layer*pl,int mspos)
{
	srs_rtmp_client *s = (srs_rtmp_client *)pl->plug_pri_data;
	if( !s ) return 0;
	if( s->running )
	{
		s->pause = 0;
	}
	else
	{
		memset(s->previous_time, 0, sizeof(s->previous_time));
		memset(s->offset_time, 0, sizeof(s->offset_time));

		s->locker	= mutex_init();
		s->rtmp_	= srs_rtmp_create( s->uri );
		s->running	= 1;
		s->worker_handle = vpc_thread_create((THREAD_ID_TYPE*)&s->pb_thread_id, SRS_RtmpStreamThread, pl);
	}
	return 1;
}

int plugin_eof_rtmp(source_layer*pl)
{
	srs_rtmp_client *s = (srs_rtmp_client *)pl->plug_pri_data;
	if( !s ) return 1;
	return 	s->eof;
}

int plugin_close_rtmp(source_layer*pl)
{
	srs_rtmp_client *s = (srs_rtmp_client *)pl->plug_pri_data;
	if( !s ) return 0;

	s->running = 0;

	/* set block flag */
	mutex_lock(s->locker);
	srs_set_quit_flag(s->rtmp_);
	mutex_unlock(s->locker);

	/* wait thread finish */
	vpc_thread_destory(&s->worker_handle, s->pb_thread_id);

	/* delete object */
	if (s->rtmp_) {
		srs_rtmp_disconnect_server(s->rtmp_);
		srs_rtmp_destroy(s->rtmp_);
		s->rtmp_ = NULL;
	}
	mutex_uninit(s->locker);
	vpc_mem_free( pl->plug_pri_data );
	pl->plug_pri_data = 0;
	return 1;
}

int plugin_seek_rtmp(source_layer*pl,int ms)
{
	srs_rtmp_client *s = (srs_rtmp_client *)pl->plug_pri_data;
	//don't support seek
	return 0;
}

int plugin_pause_rtmp(source_layer*pl)
{
	srs_rtmp_client *s = (srs_rtmp_client *)pl->plug_pri_data;
	if( !s ) return 0;
	s->pause = 1;
	return 1;
}

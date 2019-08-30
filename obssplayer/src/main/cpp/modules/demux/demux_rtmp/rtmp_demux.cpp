
#include "fa_flv2es.h"
#include "rtmp/include/rtmp.h"
#include "rtmp_demux.h"
#include "../../../core/common.h"
#include "rmH265InfoCheck.c"

#define LOG_TAG "rtmpDemux"

extern "C" {
#define  inline __inline
#include "inttypes.h"
#include "common/FFMPEG_lib/include/libavcodec/avcodec.h"
#include "../../decoder/aac/libFdk_aac/include/aacdecoder_lib.h"
}

static int plugin_open_rtmp(source_layer*pl, player_parameter *pp,FILLSAMPLE fillsample, void *key);
static int plugin_set_notify_rtmp(source_layer*pl,MSG_GATEWAY notify, void *key );
static int plugin_start_rtmp(source_layer*pl,int mspos);
static int plugin_eof_rtmp(source_layer*pl);
static int plugin_close_rtmp(source_layer*pl);
static int plugin_seek_rtmp(source_layer*pl,int ms);
static int plugin_pause_rtmp(source_layer*pl);
static int plugin_customize_rtmp(source_layer *pl, char*name, int *parm, int parm_cnt);

int parse_flv_file_header(uint8_t * header_buf)
{
	int flv_version, header_length;
	int enable_audio,enable_video;

	if(header_buf[0] != 'F' || header_buf[1] != 'L' || header_buf[2] != 'V')
	{
		printf("This is not a FLV file !!!\n");
		return -1;
	}

	flv_version = header_buf[3];

	/* always have video and audio */
	enable_audio = header_buf[4] & 0x4;
	enable_video = header_buf[4] & 0x1;

	header_length =	(header_buf[5] & 0xFF) << 24
					| (header_buf[6] & 0xFF) << 16
					| (header_buf[7] & 0xFF) << 8
					| (header_buf[8] & 0xFF);

	if(header_length != FLV_FILE_HEADER_SIZE)
	{
		//有时候 flv 头信息后面有其他的信息，这个值就不是 9 了
		vpc_printf("FLV file header information , length error !!!\n");
		return -1;
	}
	return 0;
}

int parse_flv_tag_header(rtmp_client *p, uint8_t * header_buf)
{
	p->flv_tag.tag_type  = header_buf[0];
	p->flv_tag.date_size = (header_buf[1] & 0xFF) << 16 | (header_buf[2] & 0xFF) << 8 | (header_buf[3] & 0xFF);
	p->flv_tag.timestamp = (header_buf[4] & 0xFF) << 16 | (header_buf[5] & 0xFF) << 8 | (header_buf[6] & 0xFF);
	p->flv_tag.timestamp_extended = header_buf[7];
	p->flv_tag.stream_id = (header_buf[8] & 0xFF) << 16 | (header_buf[9] & 0xFF) << 8 | (header_buf[10] & 0xFF);

	if( p->flv_tag.stream_id != 0){
		vpc_printf("The value of the StreamID should be 0 !!!\n");
	}
	return 0;
}

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
		pl->plug_pri_data = vpc_mem_alloc( sizeof(rtmp_client));
		if( !pl->plug_pri_data ) 
		{
			vpc_mem_free(pl);
			return 0;
		}
		pl->open		= plugin_open_rtmp;
		pl->set_notify	= plugin_set_notify_rtmp;
		pl->start		= plugin_start_rtmp;
		pl->close		= plugin_close_rtmp;
		pl->customize   = plugin_customize_rtmp;
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

static const node_boot_func rtmp_live_demuxer = {
	"facebac",
	"facebac RTMP demux",
	SOURCE_ID,
	RTMP_ID,
	PRIORITY_STANDARD,
	0,
	rtmp_Available,
	rtmp_CreateModule,
	rtmp_ReleaseModule
};

void rtmp_stream_register(void *ptx)
{
	RegisterTools(ptx,&rtmp_live_demuxer,0);
}

#if 0
void resync_stream_time( rtmp_client *p )
{
	int i = 0;
	TIME_TYPE new_stream_time = 0;
	for ( i=0;i<MAX_STREAM_NUM; i++)
	{
		if( p->stream[i].stream_time > new_stream_time ){
			new_stream_time = p->stream[i].stream_time;
		}
	}
	for ( i=0;i<MAX_STREAM_NUM; i++){
		p->stream[i].stream_time = new_stream_time;
		p->stream[i].prev_time_stamp = 0;
	}
}

TIME_TYPE compute_media_time(rtmp_client *p, int media, TIME_TYPE dts){
	rtmp_stream *s = &p->stream[media];
	TIME_TYPE retTime = 0;
	int av_diff = 0;

	if( !s->prev_time_stamp ){
		s->prev_time_stamp = dts;
	}

	av_diff = (int)(dts - s->prev_time_stamp);

	if( av_diff < 0){
		av_diff = av_diff;
	}
/*
// 	if( media == TM_MEDIA_AUDIO )
// 	{
// 		static FILE *fp = 0;
// 		if(!fp)
// 			fp = fopen("d:\\audio_ts.txt","w");
// 		fprintf(fp,"%d-%d=%d\r\n",dts,s->prev_time_stamp,av_diff);
// 		fflush(fp);
// 	}
// 	else if(media == TM_MEDIA_VIDEO){
// 		static FILE *fp2 = 0;
// 		if(!fp2)
// 			fp2 = fopen("d:\\video_ts.txt","w");
// 		fprintf(fp2,"%d-%d=%d\r\n",dts,s->prev_time_stamp,av_diff);
// 		fflush(fp2);
// 	}
*/
	if( abs(av_diff) > 1500 ){//1500 ms
		av_diff = s->diff;
		resync_stream_time(p);
		//printf("av_diff=%d\r\n",av_diff);
	}
// 	else if( av_diff == 0 )
// 		av_diff = 56;
// 
// 	if( media == TM_MEDIA_AUDIO && av_diff == 0 ){
// 		vpc_printf("----------audio av_diff=%d\r\n",av_diff);
// 	}
// 
// 	if( media == TM_MEDIA_VIDEO && av_diff == 0 ){
// 		vpc_printf("----------video av_diff=%d\r\n",av_diff);
// 	}

	s->prev_time_stamp = (TIME_TYPE)dts;
	retTime = s->stream_time;

	s->stream_time += av_diff;
	if( av_diff > 0 )
		s->diff = av_diff;
	return retTime;
}
#endif

void demux_h26x_xps(rtmp_client *p, int codecid, int i)
{
	int length_a, length_b;
	int num = 0;
	AVCDecoderConfigurationRecord avc_config_data ={0};

	//AVC sequence header, sps pps and vps 
	avc_config_data.configurationVersion = p->flv_tag.data[i++];
	avc_config_data.AVCProfileIndication = p->flv_tag.data[i++];
	avc_config_data.profile_compatibility = p->flv_tag.data[i++];
	avc_config_data.AVCLevelIndication = p->flv_tag.data[i++];
	avc_config_data.lengthSizeMinusOne = p->flv_tag.data[i++] & 0x3;
	avc_config_data.numOfSequenceParameterSets = p->flv_tag.data[i++] & 0x1f;

	for(num = 0;num < avc_config_data.numOfSequenceParameterSets;num++)
	{
		uint32_t sps_size = 0;
		length_a = p->flv_tag.data[i++];
		length_b = p->flv_tag.data[i++];
		sps_size = avc_config_data.sequenceParameterSetLength = (length_a << 8 ) | length_b;
		if( codecid == FLV_CODECID_H264 )
		{
			p->sequence_buf[0] = (sps_size&0xFF000000)>>24;
			p->sequence_buf[1] = (sps_size&0xFF0000)>>16;
			p->sequence_buf[2] = (sps_size&0xFF00)>>8;
			p->sequence_buf[3] = (sps_size&0xFF);
		}
		else
		{
			p->sequence_buf[0] = 0;
			p->sequence_buf[1] = 0;
			p->sequence_buf[2] = 0;
			p->sequence_buf[3] = 1;
		}
		memcpy(p->sequence_buf+4,p->flv_tag.data+i,avc_config_data.sequenceParameterSetLength);

		p->sequence_size += 4;
		p->sequence_size += sps_size;
		i += avc_config_data.sequenceParameterSetLength;
	}

	avc_config_data.numOfPictureParameterSets = p->flv_tag.data[i++] & 0xff;
	for(num = 0;num < avc_config_data.numOfPictureParameterSets;num++)
	{
		uint32_t pps_size = 0;
		length_a = p->flv_tag.data[i++];
		length_b = p->flv_tag.data[i++];
		pps_size = avc_config_data.pictureParameterSetLength = (length_a << 8 ) | length_b;

		if( codecid == FLV_CODECID_H264 )
		{
			p->sequence_buf[p->sequence_size+0] = (pps_size&0xFF000000)>>24;
			p->sequence_buf[p->sequence_size+1] = (pps_size&0xFF0000)>>16;
			p->sequence_buf[p->sequence_size+2] = (pps_size&0xFF00)>>8;
			p->sequence_buf[p->sequence_size+3] = (pps_size&0xFF);
		}
		else
		{
			p->sequence_buf[p->sequence_size+0] = 0;
			p->sequence_buf[p->sequence_size+1] = 0;
			p->sequence_buf[p->sequence_size+2] = 0;
			p->sequence_buf[p->sequence_size+3] = 1;
		}
		memcpy(p->sequence_buf+p->sequence_size+4,p->flv_tag.data+i,avc_config_data.pictureParameterSetLength);

		p->sequence_size += 4;
		p->sequence_size += pps_size;

		i += avc_config_data.pictureParameterSetLength;
	}

	if( codecid == FLV_CODECID_H265 )
	{
		avc_config_data.numOfVideoParameterSets = p->flv_tag.data[i++] & 0xff;
		for(num = 0;num < avc_config_data.numOfVideoParameterSets;num++)
		{
			uint32_t vps_size = 0;
			length_a = p->flv_tag.data[i++];
			length_b = p->flv_tag.data[i++];
			vps_size = avc_config_data.videoParameterSetLength = (length_a << 8 ) | length_b;
			p->sequence_buf[p->sequence_size+0] = 0;
			p->sequence_buf[p->sequence_size+1] = 0;
			p->sequence_buf[p->sequence_size+2] = 0;
			p->sequence_buf[p->sequence_size+3] = 1;
			memcpy(p->sequence_buf+p->sequence_size+4,p->flv_tag.data+i,avc_config_data.videoParameterSetLength);

			p->sequence_size += 4;
			p->sequence_size += vps_size;

			i += avc_config_data.videoParameterSetLength;
		}
	}
}

typedef struct cfg{
	unsigned int audio_object_type;
	unsigned int frequency;
	unsigned int channels;
}mpeg4_audio_config_t;

int detect_AAC_bitstream(rtmp_client *p, uint8_t*buf, int buf_size , mpeg4_audio_config_t *cfg)
{
	/* use ffmpeg aac decoder to detect format*/
    int ret = 0;
	avcodec_register_all();
	AVCodecContext  *pCodecCtx = 0;
	AVCodec         *pCodec = 0;
	AVFrame			*frame = 0;
    AVPacket        pkt={0};
    int i,got_frame;
    
	pCodec = avcodec_find_decoder( AV_CODEC_ID_AAC );
	if(!pCodec){
		goto Exit; // Codec not found
	}
	pCodecCtx = avcodec_alloc_context3(pCodec);
	if(!pCodecCtx){
		goto Exit;
	}

	frame	= av_frame_alloc();
	if( !frame){
		goto Exit;
	}

	if( p->stream[TM_MEDIA_AUDIO].extrasize > 0 )
	{
		pCodecCtx->extradata_size =  p->stream[TM_MEDIA_AUDIO].extrasize;
		pCodecCtx->extradata = p->stream[TM_MEDIA_AUDIO].extradata;
	}
	pCodecCtx->codec_id = pCodec->id;
	if (avcodec_open2(pCodecCtx, pCodec,0) < 0) {
		goto Exit;
	}
	av_init_packet(&pkt);
	if( p->flv_tag.date_size > 2 )
	{
		pkt.size = p->flv_tag.date_size - 2;
		pkt.data = p->flv_tag.data + 2;
	}

	avcodec_send_packet( pCodecCtx, &pkt );
	avcodec_receive_frame(pCodecCtx, frame);
//	i = avcodec_decode_audio4( pCodecCtx, frame, &got_frame,&pkt );

	if( pCodecCtx->codec_id == AV_CODEC_ID_AAC )
	{
		if (FF_PROFILE_AAC_LOW	== pCodecCtx->profile ||FF_PROFILE_AAC_MAIN == pCodecCtx->profile){
			cfg->audio_object_type  = 2;
		}
		else if (FF_PROFILE_AAC_HE == pCodecCtx->profile){
			cfg->audio_object_type = 5;
		}
		else if(FF_PROFILE_AAC_HE_V2 == pCodecCtx->profile){
			cfg->audio_object_type = 29;
		}
		else{
			cfg->audio_object_type = 2;
		}
		cfg->channels = pCodecCtx->channels;
		cfg->frequency = pCodecCtx->sample_rate;

		if( !cfg->channels ){
			cfg->channels = 1;
		}

		if( !cfg->frequency){
			cfg->frequency = 44100;
		}
        ret = 1;
	}
Exit:
	if( pCodecCtx ){
		pCodecCtx->extradata = 0;
	}
	avcodec_free_context( &pCodecCtx );
	av_frame_free( &frame );
	return ret;
}

/* buf is tag data */
int demux_tag(void *parm, uint8_t *buf, int buf_size )
{
	source_layer *pl = (source_layer*)parm;
	rtmp_client *p = (rtmp_client*)pl->plug_pri_data;
	uint8_t *previousTag = buf + (buf_size - 4);
	int i = 0;
	if( p->flv_tag.tag_type == FLV_TAG_TYPE_AUDIO )
	{
		AudioTagHeader aac_config_data = {0};
		aac_config_data.SoundFormat = ( p->flv_tag.data[i] >> 4) & 0xf;
		aac_config_data.SoundRate	= ( p->flv_tag.data[i] >> 2) & 0x3;
		aac_config_data.SoundSize	= ( p->flv_tag.data[i] >> 1) & 0x1;
		aac_config_data.SoundType	= p->flv_tag.data[i] & 0x1;

		/* aac */
		if( aac_config_data.SoundFormat == 10 ) {
			aac_config_data.AACPacketType = p->flv_tag.data[++i];
		}

		if( aac_config_data.SoundFormat == FLV_CODECID_AAC ) 
		{
			if( aac_config_data.AACPacketType == 0x00 /*&& !p->stream[TM_MEDIA_AUDIO].present*/ )  // 0x00 AAC sequence header
			{
				p->stream[TM_MEDIA_AUDIO].extrasize = p->flv_tag.date_size-2;
				memcpy(p->stream[TM_MEDIA_AUDIO].extradata,p->flv_tag.data+2, p->flv_tag.date_size-2 );
			}
			else if( aac_config_data.AACPacketType == 0x01 ) //0x01 AAC raw data
			{
				int aac_frame_size = p->flv_tag.date_size-2;

				p->flv_tag.data[0] = (aac_frame_size>>8);
				p->flv_tag.data[1] = aac_frame_size;

				/* detect aac bit stream*/
				if( !p->stream[TM_MEDIA_AUDIO].present )
				{
					int sbr = 0,ps = 0;
					/* media format */
					MEDIA_FMT media_fmt = {0};
					int cnt = media_fmt.cnt;
					mpeg4_audio_config_t audio_cfg;

					media_fmt.fmt[cnt].nType = TM_MEDIA_AUDIO;
					if(!detect_AAC_bitstream(p,p->flv_tag.data+2, aac_frame_size, &audio_cfg ) ){
						vpc_printf("can't get aac cfg\r\n");
						return -1;
					}
					
					switch (audio_cfg.audio_object_type) {
					case 29:
						media_fmt.fmt[cnt].nFourCC = AACPP_ID;
						strcpy(media_fmt.fmt[cnt].codecname,"AAC-HEv2");
						vpc_printf("reach audio spec:AAC-HE-v2\r\n");
						sbr = 1;
						ps = 1;
						break;
					case 5:
						media_fmt.fmt[cnt].nFourCC = AACP_ID;
						strcpy(media_fmt.fmt[cnt].codecname,"AAC-HEv1");
						vpc_printf("reach audio spec:AAC-HE\r\n");
						sbr = 1;
						break;
					case 2:
						media_fmt.fmt[cnt].nFourCC = AAC_ID;
						strcpy(media_fmt.fmt[cnt].codecname,"AAC-LC");
						vpc_printf("reach audio spec:AAC\r\n");
						break;
					default:
						break;
					}
					// get audio spec
					media_fmt.fmt[cnt].av.a.nBitsPerSample = 16;
					media_fmt.fmt[cnt].av.a.nChannels = audio_cfg.channels;
					media_fmt.fmt[cnt].av.a.nSamplerate = audio_cfg.frequency;

					//sample:channel:sbr:ps
					char *extra = media_fmt.fmt[cnt].extradata;
					memset(extra,0,sizeof(extra));
					media_fmt.fmt[cnt].extrasize = /*p->flv_tag.date_size-2;*/sprintf(extra,"%d/%d/%d/%d",audio_cfg.frequency,audio_cfg.channels,sbr, ps);
					//p->media_fmt.fmt[cnt].extrasize = p->flv_tag.date_size-2;

					media_fmt.cnt++;
					p->stream[TM_MEDIA_AUDIO].present = 1;
					p->notify_routine( p->key, MSG_MEDIA_FMT, (long)&media_fmt, sizeof(media_fmt));
				}

				stream_pack sp = {0};
				sp.frame_style = 'I';
				sp.buffer = p->flv_tag.data;
				sp.buf_size = p->flv_tag.date_size;

				TIME_TYPE dts = p->flv_tag.timestamp_extended;
				dts <<= 24;
				dts += p->flv_tag.timestamp;


				sp.dts = dts;//compute_media_time(p,TM_MEDIA_AUDIO, dts);
				sp.vtag.rt = vpc_gettime();
				sp.style = TM_MEDIA_AUDIO;
				p->fillsample(p->fillkey,&sp);
			}
		}
	}
	else if( p->flv_tag.tag_type == FLV_TAG_TYPE_VIDEO )
	{
		p->flv_tag.video_data.frame_type	= p->flv_tag.data[i] >> 4;
		p->flv_tag.video_data.codec_id		= p->flv_tag.data[i++] & 0xF;
		p->flv_tag.video_data.AVCPacketType = p->flv_tag.data[i++];
		p->flv_tag.video_data.CompositionTime = (p->flv_tag.data[i++] & 0xFF) << 16 | (p->flv_tag.data[i++] & 0xFF) << 8 | (p->flv_tag.data[i++] & 0xFF);
		// use i++ CompositionTime will error
		if(	p->flv_tag.video_data.codec_id != FLV_CODECID_H264 && 
			p->flv_tag.video_data.codec_id != FLV_CODECID_H265 )
		{
			vpc_printf("FLV format video coding is not avc or hevc, unable to properly parse !!! \n");
			return -1;
		}

		/* video decode config */
		if(	p->flv_tag.video_data.frame_type == 1 && 
			p->flv_tag.video_data.AVCPacketType == 0 &&
			p->flv_tag.video_data.CompositionTime == 0  )
		{
			p->sequence_size = 0;
			p->push_avc_parms = 1;
			demux_h26x_xps(p, p->flv_tag.video_data.codec_id, i);

			if( !p->stream[TM_MEDIA_VIDEO].present ){
				MEDIA_FMT media_fmt = {0};
				int cnt = media_fmt.cnt;
				media_fmt.fmt[cnt].nType = TM_MEDIA_VIDEO;
				if( p->flv_tag.video_data.codec_id == FLV_CODECID_H265 ){
					media_fmt.fmt[cnt].nFourCC = H265_ID;
					strcpy(media_fmt.fmt[cnt].codecname,"H265");
					vpc_printf("reach video spec:H265\r\n");
				}
				else{
					media_fmt.fmt[cnt].nFourCC = H264_ID;
					strcpy(media_fmt.fmt[cnt].codecname,"H264");
					vpc_printf("reach video spec:H264\r\n");
				}
				media_fmt.cnt++;
				p->stream[TM_MEDIA_VIDEO].present = 1;
				p->notify_routine( p->key, MSG_MEDIA_FMT, (long)&media_fmt, sizeof(media_fmt));
			}
		}
		else if( p->flv_tag.video_data.AVCPacketType == 1 )
		{
			int nalsize = 0;
			uint8_t *ptr = p->flv_tag.data + 5;
			int video_size = buf_size - 5 - 4; /* 5 is video header， 4 is preread previous tag size */
			uint8_t *parms_and_es = 0;
			int parms_es_size = 0;
			stream_pack sp = {0};
			int naltype = 0;
			if( buf_size < 4 ){
				return 1;
			}

			TIME_TYPE pts,dts = p->flv_tag.timestamp_extended;
			dts <<= 24;
			dts += p->flv_tag.timestamp;
			pts = dts - p->flv_tag.video_data.CompositionTime;

			sp.dts = dts;//compute_media_time(p,TM_MEDIA_VIDEO, dts);

			sp.vtag.rt = vpc_gettime();
			sp.style = TM_MEDIA_VIDEO;

			sp.frame_style = '?';

			/*
			if( p->flv_tag.video_data.frame_type == 1 ){
				sp.frame_style = 'I';
			}
			else if( p->flv_tag.video_data.frame_type == 2){
				sp.frame_style = 'P';
			}
			else if( p->flv_tag.video_data.frame_type == 3){
				sp.frame_style = 'B';
			}
			*/
			/*
			if( naltype == 6 )//SEI
			{
				nalsize = (ptr[0] & 0xFF) << 24
				| (ptr[1] & 0xFF) << 16
				| (ptr[2] & 0xFF) << 8
				| (ptr[3] & 0xFF);
				if( nalsize > video_size - 4) {
					return -1; //error
				}
				ptr += nalsize + 4;
				video_size -= nalsize + 4;
				naltype = ptr[4] & 0x1f;
			}
			*/

			if( p->flv_tag.video_data.codec_id == FLV_CODECID_H265 ){
				SLICE_TYPE type = rmFrameTypeCheck(ptr, video_size);
				if( type == I_SLICE ){
					sp.frame_style = 'I';
				}
				else if ( type == P_SLICE ){
					sp.frame_style = 'P';
				}
				else if ( type == B_SLICE ){
					sp.frame_style = 'B';
				}
				else if ( type == B_SLICE_SKIP ){
					sp.frame_style = 'B';
					sp.skippable = 1;
				}
				else{
					return -1;
				}
			}
			else{

				while (video_size>0)
				{
					nalsize = (ptr[0] & 0xFF) << 24 | (ptr[1] & 0xFF) << 16 | (ptr[2] & 0xFF) << 8 | (ptr[3] & 0xFF);
					naltype = ptr[4]&0x1f;
					if( naltype == 9 || naltype == 6 ){ //skip it
						nalsize += 4;
						ptr += nalsize;
						video_size -= nalsize;
						continue;
					}

					if( naltype == 5 || naltype == 7 || naltype == 8 ){
						sp.frame_style = 'I';
					}
					else if ( ptr[4] & 0x60){
						sp.frame_style = 'P';
					}
					else {
						sp.frame_style = 'B';
						sp.skippable = 1;
					}
					break;
				}
			}
			sp.buffer = ptr;
			sp.buf_size = video_size;

			if( p->push_avc_parms && sp.frame_style == 'I' )
			{
				// need add sps pps vps(h265 only)
				parms_es_size = p->sequence_size + video_size;
				parms_and_es = (uint8_t*)vpc_mem_alloc(parms_es_size);
				memcpy(parms_and_es,p->sequence_buf,p->sequence_size);
				memcpy(parms_and_es+p->sequence_size,ptr,video_size );

				sp.buffer = parms_and_es;
				sp.buf_size = parms_es_size;
				p->push_avc_parms = 0;
			}

// 			while ( video_size > 0)
// 			{
// 				nalsize = (ptr[0] & 0xFF) << 24
// 					| (ptr[1] & 0xFF) << 16
// 					| (ptr[2] & 0xFF) << 8
// 					| (ptr[3] & 0xFF);
// 				ptr[0] = 0x00;
// 				ptr[1] = 0x00;
// 				ptr[2] = 0x00;
// 				ptr[3] = 0x01;
// 				ptr += nalsize + 4;
// 				video_size -= nalsize+4;
// 			}
 
			p->fillsample(p->fillkey,&sp);
			if(parms_and_es){
				vpc_mem_free(parms_and_es);
			}
		}
	}
	// 	else if(p->flv_tag.tag_type == FLV_TAG_TYPE_META)
	// 	{
	// 		do_tag_onMetaData((char*)p->flv_tag.data,p->flv_tag.date_size);
	// 	}

	// check previous tag size
	p->flv_tag.previous_tag_size = (previousTag[0] & 0xFF) << 24
		| (previousTag[1] & 0xFF) << 16
		| (previousTag[2] & 0xFF) << 8
		| (previousTag[3] & 0xFF);

	if( p->flv_tag.previous_tag_size != p->flv_tag.date_size + FLV_TAG_HEADER_SIZE ){
		return -1;
	}
	return 1;
}

int demux_rtmp( memory_cache *h, void * parm)
{
	uint8_t *buff = 0;
	source_layer *pl = (source_layer*)parm;
	rtmp_client *p = (rtmp_client*)pl->plug_pri_data;
	int ret = 0;

	if( p->parse_step == 1 ){
		//read a tag
		if( memory_read(h,&buff,FLV_TAG_HEADER_SIZE) != FLV_TAG_HEADER_SIZE ) {
			return 0;
		}
		ret = parse_flv_tag_header(p, buff);
		if(ret < 0)
			return -1;

		// read a FLV tag data
		if( memory_read(h, &p->flv_tag.data, p->flv_tag.date_size + 4) != ( p->flv_tag.date_size + 4) )
		{
			memory_read(h,0, -FLV_TAG_HEADER_SIZE );
			vpc_printf("no more data, back header\r\n");
			return 0;
		}

		//get a tag so demux it
		return demux_tag( parm, p->flv_tag.data, p->flv_tag.date_size + 4);

	}
	else if( p->parse_step == 0 ){
		/* detect flv head */
		/* 9 bytes head + 4 PreviousTagSize0 UI32 Always 0*/
		if( memory_read(h, &buff,9 + 4 ) != 9 + 4 ) {
			return 0;
		}
		ret = parse_flv_file_header(buff);
		if(ret < 0)
			return(-1); //it's not a flv file
		p->parse_step = 1;
	}
	return 1;
}

void reset_stream_time(rtmp_client *p)
{
	for ( int i=0;i<MAX_STREAM_NUM; i++)
	{
		p->stream[i].prev_time_stamp = 0;
		p->stream[i].stream_time = 0;
		p->stream[i].diff = 0;
	}
}

THREAD_RETURN_TYPE VPC_API RtmpStreamThread(  THREAD_PARAM_TYPE lpParameter )
{
	/* source player */
	source_layer *pl = (source_layer*)lpParameter;

	/* */
	rtmp_client *p = (rtmp_client*)pl->plug_pri_data;

	//	RTMPPacket rtmp_pakt = {0};
	//	int temp = 0;
	int nRet = 0;
	TIME_TYPE time = 0;
	TIME_TYPE reconnect_elapsed_time = 0;

	memory_cache mc;

// 	FILE *fp=fopen("d:\\receive.flv","wb");  
// 	if (!fp){  
// 		return -1;  
// 	}  

	/* alloc memory cache */
	memory_init( &mc, 500*1024 );

	p->connect_server = 1;

	p->rtmpHandle->Link.timeout= 5; // time out 60s
	p->rtmpHandle->Link.lFlags |= RTMP_LF_LIVE;

	while( p->running )
	{  
		int buf_size = 0;
		int nRetServer = 0, nRetStream = 0;
		uint8_t *pbuf= 0;

		/* if need connect to server */
		if( p->connect_server )
		{
			if (RTMP_IsConnected(p->rtmpHandle)){
				RTMP_Close(p->rtmpHandle);
			}
			p->rtmpHandle->flag = 0;

			RTMP_Init(p->rtmpHandle);

			time = vpc_gettime();
			if (!RTMP_SetupURL(p->rtmpHandle, p->uri)) {
			}

			if ( (nRetServer = RTMP_Connect(p->rtmpHandle, NULL)) && (nRetStream = RTMP_ConnectStream(p->rtmpHandle, 0))) {
				vpc_printf("connect to server use %d ms.\r\n",vpc_gettime() - time);
				
				/* parse flv again */
				p->parse_step = 0;
				p->push_avc_parms = 1;

				/* if player request connect server, reset stream time */
				if( p->connect_server == 1 ){
					p->notify_routine( p->key, MSG_LIVE_RESET, 0, 0 );
					reset_stream_time(p);
					vpc_printf("MSG_LIVE_RESET\r\n");
				}
				p->connect_server = 0;
				reconnect_elapsed_time = 0;
			}
			else{
				// sleep
				vpc_delay(200);
				reconnect_elapsed_time += (vpc_gettime() - time);
				if( reconnect_elapsed_time > 12000){
					int n = VPC_NET_TIME_OUT;
					if( !nRetStream ) 
						n = VPC_NO_PLAY_OBJECT;
					p->notify_routine( p->key, MSG_BY_PASS, n, 0);
					break;
				}
			}
		}
		else
		{
			/* demux rtmp packet*/
			nRet = demux_rtmp( &mc, pl );
			if( nRet == -1 )
				break; // it's not a flv format
			else if( nRet == 0 ){
				pbuf = memory_move(&mc, &buf_size);
				buf_size = RTMP_Read(p->rtmpHandle, (char*)pbuf, buf_size);
				if( buf_size > 0 ){
					/* gotten some data */
					memory_fill( &mc, buf_size);
// 					fwrite(pbuf,1,buf_size,fp); 
// 					fflush(fp);
				}
				else{
					p->connect_server = 2;
				}
			}
// 			if( vpc_gettime() - time > 10000)
// 				p->connect_server = 1;
		}
	}
	memory_uninit(&mc);
	// 
	// 	do {
	// 		while (isStartPlay && RTMP_ReadPacket(rtmpHandle, &rtmp_pakt)) {
	// 			if (RTMPPacket_IsReady(&rtmp_pakt)) {
	// 				if (!rtmp_pakt.m_nBodySize)
	// 					continue;
	// 				if (rtmp_pakt.m_packetType == RTMP_PACKET_TYPE_AUDIO) {
	// 					printf("RTMP_PACKET_TYPE_AUDIO\r\n");
	// #if 0
	// 					temp = fdk_decode_audio(output_buffer, &output_size, rtmp_pakt.m_body+1, rtmp_pakt.m_nBodySize-1);
	// 					LOGI("decode size: %d", temp);
	// 					putAudioQueue(output_buffer, output_size);
	// #endif
	// 
	// 				} 
	// 				else if (rtmp_pakt.m_packetType == RTMP_PACKET_TYPE_VIDEO) {
	// 					printf("RTMP_PACKET_TYPE_VIDEO\r\n");
	// 				} else if (rtmp_pakt.m_packetType == RTMP_PACKET_TYPE_INFO) {
	// 					printf("RTMP_PACKET_TYPE_INFO\r\n");
	// 				} else if (rtmp_pakt.m_packetType == RTMP_PACKET_TYPE_FLASH_VIDEO) {
	// 					printf("RTMP_PACKET_TYPE_FLASH_VIDEO\r\n");
	// 				}
	// 				//  LOGI( "rtmp_pakt size:%d  type:%d\n", rtmp_pakt.m_nBodySize, rtmp_pakt.m_packetType);
	// 				RTMPPacket_Free(&rtmp_pakt);
	// 			}
	// 		}
	// 	} while (0);

	if (RTMP_IsConnected(p->rtmpHandle)) {
		RTMP_Close(p->rtmpHandle);
	}

	return 0;
}

//--------------------------plugin interface----------------------endif

int plugin_open_rtmp(source_layer*pl, player_parameter *pp,FILLSAMPLE fillsample, void *key)
{
	rtmp_client *s = (rtmp_client *)pl->plug_pri_data;
	if( !s ) return 0;
	s->fillsample = fillsample;
	s->fillkey	= key;
	strcpy(s->uri, pp->play_url);
	return 1;
}

int plugin_set_notify_rtmp(source_layer*pl,MSG_GATEWAY notify, void *key )
{
	rtmp_client *s = (rtmp_client *)pl->plug_pri_data;
	if( !s ) return 0;
	s->notify_routine = notify;
	s->key = key;
	return 1;
}

int plugin_start_rtmp(source_layer*pl,int mspos)
{
	LOGE("开始rtmp初始化");
	rtmp_client *s = (rtmp_client *)pl->plug_pri_data;
	if( !s ) return 0;
	if( s->running )
	{
		s->pause = 0;
	}
	else
	{
		s->running = 1;
		s->rtmpHandle = RTMP_Alloc();
		RTMP_Init(s->rtmpHandle);
		s->worker_handle = vpc_thread_create((THREAD_ID_TYPE*)&s->pb_thread_id,RtmpStreamThread,pl);
	}
	return 1;
}

int plugin_customize_rtmp(source_layer *pl, char*name, int *parm, int parm_cnt)
{
	return 0;
#if 0
	if( !strncmp(name,"buffer",6) ){
		rtmp_client *p = (rtmp_client*)pl->plug_pri_data;
		if( !p->connect_server){
			p->connect_server = 1;
		}
		return 1;
	}
	return 0;
#endif
}

int plugin_eof_rtmp(source_layer*pl)
{
	rtmp_client *s = (rtmp_client *)pl->plug_pri_data;
	if( !s ) return 1;
	return 	s->eof;
}

int plugin_close_rtmp(source_layer*pl)
{
	rtmp_client *s = (rtmp_client *)pl->plug_pri_data;
	if( !s ) return 0;
	s->rtmpHandle->flag = 1; //rtmp exit
	s->running = 0;
	vpc_thread_destory(&s->worker_handle, s->pb_thread_id);
	RTMP_Free(s->rtmpHandle);
	vpc_mem_free( pl->plug_pri_data );
	pl->plug_pri_data = 0;
	return 1;
}

int plugin_seek_rtmp(source_layer*pl,int ms)
{
	rtmp_client *s = (rtmp_client *)pl->plug_pri_data;
	//don't support seek
	return 0;
}

int plugin_pause_rtmp(source_layer*pl)
{
	rtmp_client *s = (rtmp_client *)pl->plug_pri_data;
	if( !s ) return 0;
	s->pause = 1;
	return 1;
}


//////////////////////////////////////////////////////////////////////////
void memory_init( memory_cache *h, int cache_size )
{
	h->buf_raw = (uint8_t*)vpc_mem_alloc(cache_size);
	h->buf_size = cache_size;

	h->start = h->buf_raw;
	h->end = h->buf_raw;
}

void memory_reset( memory_cache *h )
{
	h->start = h->buf_raw;
	h->end = h->buf_raw;
}


uint8_t * memory_move( memory_cache *h, int *avail_size)
{
	int move_size = h->end - h->start;
	if( move_size ){
		memmove(h->buf_raw, h->start, move_size );
	}
	h->start = h->buf_raw;
	h->end  = h->buf_raw + move_size;

	*avail_size = h->buf_size - move_size;
	return h->end;
}

int memory_fill( memory_cache *h, int fill_size)
{
	h->end += fill_size;
	return h->end - h->start;
}

int memory_read( memory_cache *h, uint8_t **buf, int size )
{
	if( size > 0 )
	{
		int avail_size = h->end - h->start;
		if( avail_size >= size )
		{
			if( buf ) {
				*buf = h->start;
				//memcpy(buf,h->start, size);
				h->start += size;
				return size;
			}
		}
	}
	else{
		h->start += size;
	}
	return 0;
}

int memory_read_copy( memory_cache *h, uint8_t *buf, int size )
{
	if( size > 0 )
	{
		int avail_size = h->end - h->start;
		if( avail_size >= size )
		{
			if( buf)
			{
				memcpy(buf,h->start, size);
				h->start += size;
				return size;
			}
		}
	}
	else{
		h->start += size;
	}
	return 0;
}


void memory_uninit(memory_cache *h)
{
	vpc_mem_free(h->buf_raw);
}

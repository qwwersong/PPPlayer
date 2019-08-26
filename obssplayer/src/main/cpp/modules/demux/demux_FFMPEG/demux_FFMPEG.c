#include "../../../core/common.h"

#if ENABLE_FFMPEG

#define  inline __inline

#include "inttypes.h"
#include "common/FFMPEG_lib/include/libavcodec/avcodec.h"
#include "common/FFMPEG_lib/include/libavutil/avstring.h"
#include "common/FFMPEG_lib/include/libavutil/mathematics.h"
#include "common/FFMPEG_lib/include/libavutil/pixdesc.h"
#include "common/FFMPEG_lib/include/libavutil/imgutils.h"
#include "common/FFMPEG_lib/include/libavutil/dict.h"
#include "common/FFMPEG_lib/include/libavutil/avutil.h"
#include "common/FFMPEG_lib/include/libavutil/parseutils.h"
#include "common/FFMPEG_lib/include/libavutil/samplefmt.h"
#include "common/FFMPEG_lib/include/libavutil/avassert.h"
#include "common/FFMPEG_lib/include/libavutil/time.h"
#include "common/FFMPEG_lib/include/libavformat/avformat.h"
#include "common/FFMPEG_lib/include/libavutil/opt.h"
#include "common/FFMPEG_lib/include/libavcodec/avfft.h"

///////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////


#define	MAX_URL_SIZE		2000
#define DEFAULT_TIMEOUT_DURATION    (30*1000)/* ms ,default timeout*/

#define SKIP_FIRST_I_FRAME	1


/* *****************************************************************************
 
 _FF_MODIFIED_


 ******************************************************************************/

#define _INTRE_AUDIO_FRAMEBUFSIZE_  (4096)



char * nowTime()
{
    static char timeString[128]={0x00};
    
    time_t nowtime;
    struct tm *timeinfo;
    time( &nowtime );
    timeinfo = localtime( &nowtime );
    
    sprintf(timeString, "[%d-%d-%d,%d:%d:%d]",timeinfo->tm_year,timeinfo->tm_mon,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
    
    return timeString;
}
#ifdef _FF_DEBUG_
#define sLog(msg,...)    printf(msg,##__VA_ARGS__)

#else

#define sLog(...)

#endif



typedef struct ffmpeg_stream
{
    /* media type */
    int		media_type;
    
    /* time stamp */
    TIME_TYPE	prev_dts;
    // 	TIME_TYPE	prev_pts;
    
    int			av_diff;
    
    /* stream time */
	TIME_TYPE  stream_time;

	/* codec */
    enum AVCodecID	codec_id;
    
    /* codec extradata and length*/
	uint8_t *	extradata;
	int			extradata_size;
    
    /* internal frame buffer,now for audio-aac assemble */
    uint8_t*    inter_framebuf;
	int			inter_alloc_size;

    int			isFirstPkt;   
}ffmpeg_stream;

typedef struct FFMPEG_demux
{
    int running;
    //http server
    char uri[MAX_URL_SIZE];
    
    ffmpeg_stream stream[MAX_STREAM_NUM];
    
    
    int stream_map[6];
    
	TIME_TYPE file_length;

	int buffer_limit;
    TIME_TYPE request_seek_pos;
    
    
    int in_seeking;
    
    int eof;
    int pause;
    
    //worker thread
    THREAD_HANDLE_TYPE	worker_handle;
    THREAD_ID_TYPE pb_thread_id;
    
    /*notify callback*/
    MSG_GATEWAY notify_routine;
    void *key;
    
    /*data push callback*/
    FILLSAMPLE fillsample;
    void *fillkey;
    
    
    TIME_TYPE   timeout_from;/* ms from when to check*/
    TIME_TYPE   timeout_duration;/* ms for timeout*/
	int			wait_i_frame;
    
    int64_t     startpts;/* sometimes packet ts from ffmpeg not start from 0*/
    int         time_corrected; /* wheater startpts is corrected */
    
    AVCodecParserContext *parser;
    AVFormatContext *ic;
	
	int		reset_network;

}FFMPEG_demux;

static int plugin_open_avformat(source_layer*pl, player_parameter *pp,FILLSAMPLE fillsample, void *key);
static int plugin_set_notify_avformat(source_layer*pl,MSG_GATEWAY notify, void *key );
static int plugin_start_avformat(source_layer*pl,int mspos);
static int plugin_eof_avformat(source_layer*pl);
static int plugin_close_avformat(source_layer*pl);
static int plugin_seek_avformat(source_layer*pl,int ms);
static int plugin_pause_avformat(source_layer*pl);
static int plugin_customize_avformat(source_layer *pl, char*name, int *parm, int parm_cnt);

static void my_log(void* context, int level, const char* format, va_list args)
{

    
}

static int avformat_Available( const long parm )
{
	const char *url = (const char*)parm;
	if( !url ) return 0;

	//HLS
	if( !strncmp( url, "http://",7) && strstr(url, ".m3u8") ){
		return 1;
	}
    
    //http progressive download
    if( !strncmp( url, "http://",7) && strstr(url, ".mp4") ){
        return 1;
    }

    //HLS
//     if( !strncmp( url, "https://",8) && strstr(url, ".m3u8") ){
//         return 1;
//     }
    
	//RTMP
	if( !strncmp( url, "rtmp://",7)){
		return 1;
	}
    
    //rtsp
    if( !strncmp( url, "rtsp://",7)){
        return 1;
    }

	//udp
	if( !strncmp( url, "udp://",6)){
		return 1;
	}   

    //local
    if( !strncmp( url, "/",1) || strstr(url,".mp4") || strstr(url,".mp3")){
        return 1;
    }

	return 0;
}

static void* avformat_CreateDevice(const void * parm, int size )
{
	source_layer *pl = vpc_mem_alloc(sizeof(source_layer));
	if( pl ) 
	{
		pl->plug_pri_data = vpc_mem_alloc( sizeof(FFMPEG_demux));
		if( !pl->plug_pri_data ) 
		{
			vpc_mem_free(pl);
			return 0;
		}
		pl->open			= plugin_open_avformat;
		pl->set_notify		= plugin_set_notify_avformat;
		pl->start			= plugin_start_avformat;
		pl->close			= plugin_close_avformat;
		pl->seek			= plugin_seek_avformat;
		pl->pause			= plugin_pause_avformat;
// 		pl->customize		= plugin_customize_avformat;
		pl->eof				= plugin_eof_avformat;
        
	}
	return pl;
}

static void avformat_Delete(void *p)
{
	vpc_mem_free(p);
}

static const node_boot_func ffmpeg_avformat =
{
	"FFMPEG 3.3.9",
	"avformat",
	SOURCE_ID, 
	AVFORMAT_ID,
	PRIORITY_STANDARD,
	0,
	avformat_Available,
	avformat_CreateDevice,
	avformat_Delete,
};


void ffmpeg_avformat_register(void *pc )
{
	RegisterTools(pc, &ffmpeg_avformat,0);
}

static int ffmpeg_interrupt_cb(void *ctx)
{
    FFMPEG_demux *p = (FFMPEG_demux*)ctx;
    if ( p->timeout_from >0)
    {
        TIME_TYPE ms_current = vpc_gettime();
        TIME_TYPE ms_gap = ms_current-p->timeout_from;
        //sLog("\n[%s]--- ffmpeg_interrupt_cb()----ms_gap=%d\n ",nowTime(),ms_gap);
        if (ms_gap > p->timeout_duration)
        {
            sLog("\n--- ffmpeg_interrupt_cb()----time out\n ");
            return 1;
        }
    }
	return !p->running;
}

static int parse_stream_info(source_layer *pl, AVFormatContext *ic)
{
		 
    int cnt = 0,i;
    ffmpeg_stream *s = 0;
    
    MEDIA_FMT mft={0};
    FFMPEG_demux *p = (FFMPEG_demux*)pl->plug_pri_data;
    
    for(i=0; i<ic->nb_streams && cnt < MAX_STREAM_NUM; i++)
    {
		AVStream *st= ic->streams[i];
		AVCodecContext *codec= st->codec;

		/* get stream */
        s = &p->stream[cnt];
        
        /* set stream default time */
        s->prev_dts = 0;
        s->isFirstPkt = 1;
        
        // 		s->prev_pts = 0;
        s->av_diff = 0;
        s->stream_time = 0;

        switch(codec->codec_type){
		case AVMEDIA_TYPE_AUDIO:{

			int sbr=0;
			int ps=0;

			s->media_type = TM_MEDIA_AUDIO;
			s->codec_id = codec->codec_id;
                
                /* only support AAC*/
                if( codec->codec_id == AV_CODEC_ID_AAC )
                {
					if (FF_PROFILE_AAC_LOW	== codec->profile ||FF_PROFILE_AAC_MAIN == codec->profile)
					{
						mft.fmt[cnt].nFourCC = AAC_ID; //AAC_SW_ID; //AACPP_ID;//
						strcpy( mft.fmt[cnt].codecname, "AAC_LC");
					}
					else if (FF_PROFILE_AAC_HE == codec->profile)
					{
						mft.fmt[cnt].nFourCC = AACP_ID;//
						strcpy( mft.fmt[cnt].codecname, "AAC_HE");
						sbr = 1;
						ps =0;
					}
					else if(FF_PROFILE_AAC_HE_V2 == codec->profile)
					{
						mft.fmt[cnt].nFourCC = AACPP_ID;//
						strcpy( mft.fmt[cnt].codecname, "AAC_HEv2");
						sbr = 1;
						ps = 1;
					}
				}
				else if( codec->codec_id == AV_CODEC_ID_MP3 ) 
				{
					mft.fmt[cnt].nFourCC = MP3_ID;//
					strcpy( mft.fmt[cnt].codecname, "mp3");
				}
				else
                {
					sLog("\n----Unknow audio!!!!!!!---\n");
                    st->discard= AVDISCARD_ALL;
                    continue;
                }
                
                if(codec->extradata_size)
                {
                    if (s->extradata)
                    {
                        vpc_mem_free(s->extradata);
                        s->extradata = 0;
                    }
                    
                    s->extradata_size = codec->extradata_size;
                    s->extradata = vpc_mem_alloc(s->extradata_size);
                    memcpy(s->extradata,codec->extradata, codec->extradata_size);
                }
                
				s->inter_framebuf = vpc_mem_alloc(_INTRE_AUDIO_FRAMEBUFSIZE_);
				s->inter_alloc_size = _INTRE_AUDIO_FRAMEBUFSIZE_;
				mft.fmt[cnt].nType	= TM_MEDIA_AUDIO;
                
				if( codec->channels > 2 )
					mft.fmt[cnt].av.a.nChannels	= 1;
				else
					mft.fmt[cnt].av.a.nChannels	= codec->channels;
                if(codec->bits_per_coded_sample>0)
                    mft.fmt[cnt].av.a.nBitsPerSample = codec->bits_per_coded_sample;
                else
                    mft.fmt[cnt].av.a.nBitsPerSample = 16;
                
                mft.fmt[cnt].av.a.nSamplerate = codec->sample_rate;
                
                if(codec->codec_id == AV_CODEC_ID_AAC )
                {
					char *extra = mft.fmt[cnt].extradata;
                    //sample:channel:sbr:ps
                    mft.fmt[cnt].extrasize = sprintf(extra,"%d/%d/%d/%d",codec->sample_rate,codec->channels,sbr,ps);
					// ffmpeg audio decoder need it.
					//mft.fmt[cnt].extradata = codec->extradata;
					//mft.fmt[cnt].extrasize = codec->extradata_size;
                    
                }
                p->stream_map[i] = cnt;
                cnt++;
               // sLog("\n-------------[AVMEDIA_TYPE_AUDIO, index=%d]\n",i);
                break;
            }
                
            case AVMEDIA_TYPE_VIDEO:{
				s->media_type = TM_MEDIA_VIDEO;
				s->codec_id = codec->codec_id;
				if(codec->codec_id == AV_CODEC_ID_H264 )
				{
					mft.fmt[cnt].nFourCC = H264_ID;
					strcpy( mft.fmt[cnt].codecname, "H264");

					p->parser = av_parser_init(codec->codec_id);
					if(p->parser)
						p->parser->flags = PARSER_FLAG_COMPLETE_FRAMES;
                }
                
                else if(codec->codec_id == AV_CODEC_ID_H265)
                {
                    mft.fmt[cnt].nFourCC = H265_ID;
                    strcpy( mft.fmt[cnt].codecname, "H265");
					p->parser = av_parser_init(codec->codec_id);
					if(p->parser)
						p->parser->flags = PARSER_FLAG_COMPLETE_FRAMES;
               }
                else{
                    st->discard= AVDISCARD_ALL;
                    continue;
                }
                
                if(codec->extradata_size)
                {
                    if (s->extradata)
                    {
                        vpc_mem_free(s->extradata);
                        s->extradata = 0;
                    }
                    
                    s->extradata_size = codec->extradata_size;
                    s->extradata = vpc_mem_alloc(s->extradata_size);
                    memcpy(s->extradata,codec->extradata, codec->extradata_size);
                }
                mft.fmt[cnt].nType			= TM_MEDIA_VIDEO;
                mft.fmt[cnt].av.v.nWidth	= codec->width;
                mft.fmt[cnt].av.v.nHeight	= codec->height;

				if( s->extradata_size < sizeof(mft.fmt[cnt].extradata))
				{
					memcpy( mft.fmt[cnt].extradata, s->extradata, s->extradata_size );
					mft.fmt[cnt].extrasize	= s->extradata_size;
				}
				p->wait_i_frame = SKIP_FIRST_I_FRAME;
                p->stream_map[i] = cnt;
                cnt++;
                //sLog("\n-------------[AVMEDIA_TYPE_VIDEO, index=%d]\n",i);
                break;
            }
            default:
                st->discard= AVDISCARD_ALL;
        }
    }

	//discard left stream
	for(;i<ic->nb_streams; i++)
	{
		AVStream *st= ic->streams[i];
// 		AVCodecContext *codec= st->codec;
		st->discard = AVDISCARD_ALL;
	}
    
    if(ic->duration != AV_NOPTS_VALUE)
    {
        p->file_length = (TIME_TYPE)(ic->duration/1000);
        if(p->file_length<100)//
            p->file_length = 0;
		else
			p->wait_i_frame = 0;
    }
    else
    {
        p->file_length = 0;
    }
    mft.file_length = p->file_length;
    mft.cnt = cnt;
  
    if(cnt > 0 )
    {
		p->notify_routine( p->key, MSG_MEDIA_FMT, (long)&mft, sizeof(mft));
		return 1;
	}
	return -1;
}

static int plugin_customize_avformat(source_layer *pl, char*name, int *parm, int parm_cnt)
{
	if( !strncmp(name,"reset",5)){
		FFMPEG_demux *p = (FFMPEG_demux*)pl->plug_pri_data;
		if( !p->file_length ){
			p->reset_network = 1;
			return 1;
		}
	}
	return 0;
}

static int process_pause(FFMPEG_demux *p,AVFormatContext *ic, int paused)
{
	if( p->pause ==1 && !paused ){
		av_read_pause(ic);
		paused =1;
	}
	if( p->pause == 0 && paused){
		av_read_play(ic);
		paused = 0;
	}
	return paused;
}


static void reset_timeout(FFMPEG_demux *s)
{
    s->timeout_from =0;
    s->timeout_duration = DEFAULT_TIMEOUT_DURATION;
    
}

/** 
 * fetch one ADTS frame 
 */  
static size_t get_one_ADTS_frame_size(unsigned char* buffer, size_t buf_size)  
{  
    size_t size = 0;  
  
    while(1)  
    {  
        if(buf_size  < 7 )  
        {  
            return 0;  
        }  
  
        if((buffer[0] == 0xff) && ((buffer[1] & 0xf0) == 0xf0) )  
        {  
            size |= ((buffer[3] & 0x03) <<11);     //high 2 bit  
            size |= buffer[4]<<3;                //middle 8 bit  
            size |= ((buffer[5] & 0xe0)>>5);        //low 3bit  
            break;  
        }  
    }  
  
    if(buf_size < size) {  
        return 0;  
    }
	return size;
} 

static uint8_t *copy_to_inter_framebuf(ffmpeg_stream *s, uint8_t *src, int size)
{
	unsigned short frameSize = (unsigned short)size;

	/* if data is too large, free old, alloc new one*/
	if( s->inter_alloc_size < size + 2){
		vpc_mem_free(s->inter_framebuf);
		
		s->inter_framebuf = vpc_mem_alloc( size+2 );
		s->inter_alloc_size = size + 2;
	}
	s->inter_framebuf[0] = (frameSize >> 8);
	s->inter_framebuf[1] = frameSize;

	memcpy(s->inter_framebuf+2, src, frameSize);
	return s->inter_framebuf;
}

static void ff_fillsample(FFMPEG_demux *p,AVPacket *packet,int64_t pts,int64_t dts)
{
    /*find stream*/
    ffmpeg_stream *s = &p->stream[p->stream_map[packet->stream_index]];
    if(s)
    {
        stream_pack sp = {0};
        int av_diff = 0;
        
        //if(!s->prev_dts)
        if(s->isFirstPkt)
        {
            s->isFirstPkt = 0;
            s->prev_dts = (TIME_TYPE)dts;
        }
        
        av_diff = (int)(dts - s->prev_dts);
        if(abs(av_diff)>500){//500 ms
            av_diff = s->av_diff;
        }
        
		s->prev_dts = (TIME_TYPE)dts;
		s->stream_time += av_diff;
		s->av_diff = av_diff;
        
        if(s->media_type == TM_MEDIA_AUDIO  )
        {
			if( packet->size > 0 )
			{
				unsigned short frameSize=0;/*must two bytes*/
				if( s->codec_id == AV_CODEC_ID_AAC )
				{
					int bit_crc=0;

					unsigned char adts_head[7]={0x00};
					unsigned short syn_word=0;
					memcpy(adts_head, packet->data, 7);

					syn_word |= (adts_head[0]<<8);
					syn_word |=adts_head[1];
					syn_word >>=4;

					if (0xfff == syn_word) /*adts*/
					{
						bit_crc = adts_head[1]&0x01;
						//--
						//skip adts head
						sp.buffer = packet->data+7;;
						sp.buf_size = get_one_ADTS_frame_size(packet->data,packet->size)-7;
						sp.frame_style = 'I';
						if (0 == bit_crc) // skip 2 bytes
						{
							sp.buffer = sp.buffer+2;
							sp.buf_size -=2;
						}
					}
					else
					{
						//
						sp.buffer = packet->data;
						sp.buf_size = packet->size;
						sp.frame_style = 'I';
					}
					sp.buffer = copy_to_inter_framebuf(s,sp.buffer, sp.buf_size);
					sp.buf_size+=2;

				}
				else if( s->codec_id == AV_CODEC_ID_MP3 )
				{
					sp.buffer = copy_to_inter_framebuf(s, packet->data, packet->size);
					sp.buf_size = packet->size + 2;
					sp.frame_style = 'I';
				}
			}
			else
			{
				printf("\n Warning:Invalid Audio Data < 0 byte ... \n");
			}
		}
        else if( s->media_type == TM_MEDIA_VIDEO )
        {
        
			int size;
			uint8_t *data;
			AVStream *st = p->ic->streams[packet->stream_index];
			av_parser_parse2(p->parser, st->codec,
				&data, &size, packet->data, packet->size,
				AV_NOPTS_VALUE, AV_NOPTS_VALUE, -1);
			if( AV_PICTURE_TYPE_B == p->parser->pict_type )
			{
				sp.frame_style = 'B';
			}
			else if( AV_PICTURE_TYPE_I == p->parser->pict_type )
			{
				if( p->wait_i_frame ){
					p->wait_i_frame -= 1;
				}
				sp.frame_style = 'I';
			}
			else if( AV_PICTURE_TYPE_P == p->parser->pict_type )
			{
				sp.frame_style = 'P';
			}
			else
			{
				sp.frame_style = 'I';
			}
			sp.buffer = packet->data;
			sp.buf_size = packet->size;
#if 0
			{
				static TIME_TYPE pre = 0;
				int diff = 0;
				if(!pre) pre = s->stream_time;
				diff = s->stream_time - pre;
				pre = s->stream_time;

				printf("video ts diff=%d,now=%d\r\n",diff,pre);
			}
#endif
		}

		sp.dts = (TIME_TYPE)s->stream_time;
		sp.vtag.rt = vpc_gettime();
		sp.style = s->media_type;

		if( !p->wait_i_frame ){
			p->fillsample(p->fillkey,&sp);
		}
		else{
			s->stream_time = 0;
		}

	}
}

static int ff_hls_mul(AVFormatContext* ic )
{
    
    int n_prog=0;
    //-------------------------------------------------------------
    
    
    if (ic->nb_programs)
    {
        int j, k;
        
        for (j = 0; j < ic->nb_programs; j++)
        {
            
            AVProgram * prog = ic->programs[j];
            //printf("\n--[check_media_info]--2--ic->programs[%d]->nb_stream_indexes=%d \n",j,ic->programs[j]->nb_stream_indexes);
            
            for (k = 0; k < prog->nb_stream_indexes; k++)
            {
                int index = prog->stream_index[k];
                AVStream *st = ic->streams[index];
                AVCodecContext *dec_ctx = st->codec;
                
                if( dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO )
                {
                    
                    int bt = 0;
                    AVDictionaryEntry* vbTag = av_dict_get(prog->metadata, "variant_bitrate", NULL, 0);
                    if( vbTag )
                    {
                        bt = atoi(vbTag->value);
                    }else
                    {
                        int bt = dec_ctx->bit_rate;
                        if( bt <= 0 )
                            bt = ic->bit_rate;
                    }
                    
                    n_prog++;
                    
                    break;
                }
            }
            
        }
    }
    
    sLog("\n---ff_hls_mul(),n_prog=%d----\n",n_prog);
    
    if (n_prog >1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
    
}

int near_to_eof(FFMPEG_demux *p)
{
	if( p->file_length)
	{
		int i = 0;	
		TIME_TYPE push_stream_time = 0;
		ffmpeg_stream *s = 0;
		for (i=0;i<MAX_STREAM_NUM;i++)
		{
			s = &p->stream[i];
			if(s && (s->prev_dts > push_stream_time)){
				push_stream_time = s->prev_dts;
			}
		}
		if( (push_stream_time + 5000) > p->file_length)
			return 1;
	}
	return 0;
}


THREAD_RETURN_TYPE VPC_API DemuxStreamThread(  THREAD_PARAM_TYPE lpParameter )
{
	source_layer *pl = (source_layer*)lpParameter;
	FFMPEG_demux *p = (FFMPEG_demux*)pl->plug_pri_data;

	AVFormatContext *ic = NULL;
	AVPacket pkt={0};
	int ret = 0,err, i;
	ffmpeg_stream *s;
	int paused = 0;
	int error_time = 0;
	int inited = 0;
	TIME_TYPE time;

Retry:
	avformat_network_init();
	av_register_all();
	ic = avformat_alloc_context();
		 
	time = vpc_gettime();
	p->ic = ic;
	p->ic->flags |= AVFMT_FLAG_CUSTOM_IO;

	ic->interrupt_callback.callback = ffmpeg_interrupt_cb;
	ic->interrupt_callback.opaque = p;
//   	ic->probesize = 500 *1024;
//  	ic->max_analyze_duration = 3 * AV_TIME_BASE;


#ifdef _FF_DEBUG_
	av_log_set_level(AV_LOG_WARNING);
	av_log_set_flags(AV_LOG_SKIP_REPEATED);
#else
	av_log_set_callback(my_log);
#endif
	p->timeout_from = vpc_gettime();
	err = avformat_open_input(&ic, p->uri, 0, 0);
	if (err < 0)
    {
		if( err == AVERROR_HTTP_NOT_FOUND || err == AVERROR_PROTOCOL_NOT_FOUND || err == -2 )
		{
			p->notify_routine( p->key, MSG_BY_PASS, VPC_NO_PLAY_OBJECT,0);
		}
		else  if( err == -5 )
        {
            p->notify_routine( p->key, MSG_BY_PASS, VPC_NET_TIME_OUT,0);
        }

        else
        {
            p->notify_routine( p->key, MSG_BY_PASS, VPC_NETWORK_ERROR,0);
        }

        {
			int a = 0;
			char szError[32];
			av_strerror(err,szError,32);
			a = 2;
            printf("\n--------------\n\nErron Reason:%s\n-----------------\n\n",szError);
		}
		goto Exit; 
	}
/*	printf("connect to server use %d\r\n",vpc_gettime() - time);*/
	time = vpc_gettime();
	ret = avformat_find_stream_info(ic,NULL);
	if( ret<0 ){
        p->notify_routine( p->key, MSG_BY_PASS, VPC_MEDIA_SPEC_ERROR,0);
        goto Exit;
	}
 	printf("avformat_find_stream_info  use %d\r\n",vpc_gettime() - time);
    if (ff_hls_mul(ic))
    {
        // hls mul program, error
        p->notify_routine( p->key, MSG_BY_PASS, VPC_MEDIA_SPEC_ERROR,0);
        goto Exit;//
    }
    
    if( !inited ){
		if( parse_stream_info(pl,ic)< 0)
			goto Exit;
	}

	if( ic->start_time != AV_NOPTS_VALUE){
		AVRational t;
		t.num = 1;
		t.den = AV_TIME_BASE;
		p->startpts = ((double)ic->start_time * av_q2d(t))*1000;
	}

	inited = 1;
    
	while( p->running )
	{
        TIME_TYPE buffer_time = 0;
        int64_t pts, dts;
		s = 0;

		if( p->reset_network ){
			p->reset_network = 0;
			printf("++++++++++++++++++++++reset network++++++++++++++++++\r\n");
			goto Exit;
		}

		// seeking
		if( p->request_seek_pos )
		{
			int64_t seek_time = p->request_seek_pos;
            if (1==seek_time){
                seek_time = 0;// must
            }
            else{
                seek_time *= 1000;
            }

			if( !p->file_length ){ //live can't support seek.
				p->request_seek_pos = 0;
				continue;
			}

			if( p->file_length <= p->request_seek_pos+2000 ){
				p->notify_routine( p->key, MSG_AFTER_SEEK, p->request_seek_pos, 0 );
				p->request_seek_pos = 0;
				p->in_seeking = 0;
				p->eof = 1;
				continue;
			}
            
            /*
            if( ic->start_time != AV_NOPTS_VALUE)
            {
                if (1 == p->time_corrected)
                {
                    seek_time += p->startpts*AV_TIME_BASE/1000;
                }
                else
                {
                    seek_time += ic->start_time;
                }
                
            }
             */
			if( ic->start_time != AV_NOPTS_VALUE){
				seek_time += ic->start_time;
			}

            sLog("\n---while seeking:-------p->request_seek_pos=%d,seek_time=%lld,ic->start_time=%lld,p->startpts=%lld  \n",
                 p->request_seek_pos,seek_time,ic->start_time,p->startpts );
            
            p->timeout_from = vpc_gettime(); //20150721,reset overtime check
            //ret = av_seek_frame(ic, -1, seek_time,    0 );
            ret = av_seek_frame(ic, -1, seek_time, 0 );
            
			if(ret >= 0)
			{
				buffer_time = 0;
                p->in_seeking = 1;
                p->eof = 0;
                p->request_seek_pos = 0;//when seek success,then reset,2015.6.15
            }
			else
            {
                p->notify_routine( p->key, MSG_BY_PASS, VPC_NETWORK_ERROR,0);
                goto Exit;
            }
		}
        
		paused = process_pause(p,ic,paused);
		if( paused)
		{
			vpc_delay(100);
			continue;
		}


		if(!p->in_seeking){
			p->notify_routine( p->key, MSG_GET_BUF_TIME, (long)&buffer_time,0);
        }
       // printf("\n------buffer_time=%d ms ----------\n",buffer_time);
        
		if( buffer_time >= p->buffer_limit){
			vpc_delay(100);
			continue;
		}

		/*get frame,audio or video*/
        p->timeout_from = vpc_gettime();
        ret = av_read_frame(ic,&pkt);

// 		{
// 			int a = 0;
// 			char szError[32];
// 			av_strerror(ret,szError,32);
// 			printf("eer=%s\r\n",szError);
// 		}

		if( ret < 0 )
		{
			if( ret == AVERROR_EOF ){
				int ret = AVERROR_EOF;
			}
			if((ret == AVERROR_EOF || avio_feof(ic->pb)))
			{
				if( near_to_eof(p) || p->in_seeking )
				{
					p->eof = 1;
					error_time = 0;
					if(p->in_seeking)
					{
						p->notify_routine( p->key, MSG_AFTER_SEEK, 0, 0 );
						p->in_seeking = 0;
					}
				}
				else
				{
					vpc_delay(200);
					error_time += 200;
					if( error_time > 200*5*12 ){
						p->notify_routine( p->key, MSG_BY_PASS, VPC_NET_TIME_OUT,0);
						error_time = 0;
					}
				}
			}
			//else
			//{
			//	p->notify_routine( p->key, MSG_BY_PASS, VPC_NET_TIME_OUT,0);
			//}
			continue;
		}
		error_time = 0;

        /*find stream*/
         s = &p->stream[p->stream_map[pkt.stream_index]];
        
        if(s)
        {
            if(pkt.dts != AV_NOPTS_VALUE)
            {
                dts = 1000*pkt.dts * av_q2d(ic->streams[pkt.stream_index]->time_base);

                //sLog("\n-----New------------------Receive Pkt[stream_index=%d]:---pkt.dts=%lld ,ms=%lld-------\n",pkt.stream_index,pkt.dts,dts);
                if (dts < p->startpts)
                {
					//printf("\nWarning,dts =%lld is smaller than p->startpts =%lld\n",dts, p->startpts);
                   p->startpts = dts;//why?
                    
                    p->time_corrected = 1;
                }
                dts -= p->startpts;
               
				//if(dts==0)
               // printf("\n-----------------------Receive Pkt:---ts=%lld,-------\n",dts);

            }
            else if(pkt.pts != AV_NOPTS_VALUE)
            {
				pts = 1000*pkt.pts * av_q2d(ic->streams[pkt.stream_index]->time_base);
				pts -= p->startpts;

				dts = pts;
            }
            else{
				pts = 0;
                dts = 0;
            }
        }

      //  printf("\n----dts=%lld,pkt.dts=%lld,s->media_type=%d, pkt size=%d\n",dts,pkt.dts,s->media_type,pkt.size);
        
        if(p->in_seeking ) //dts must have value
        {
			p->in_seeking = 0;

			for (i=0;i<MAX_STREAM_NUM;i++)
			{
				p->stream[i].prev_dts = 0;
				p->stream[i].isFirstPkt = 1;
				p->stream[i].stream_time = 0;
			}
			p->notify_routine( p->key, MSG_AFTER_SEEK, dts, 0 );
        }
        ff_fillsample(p, &pkt, pts, dts);
        av_free_packet(&pkt);
        
        if( p->file_length )
        {
            if(dts+100> p->file_length){
                p->eof = 1;
                p->notify_routine( p->key, MSG_END_OF_FILE, 0,0);
           }
        }
	}
    
Exit:
	/* close decoder for each stream */
    if (ic)
    {
// 		for (i = 0; i < ic->nb_streams; i++){
// 			if (ic->streams[i]->codec->codec_id != AV_CODEC_ID_NONE)
// 				avcodec_close(&ic->streams[i]->codec);
// 		}
        avformat_close_input(&ic);
		if( p->running )
			goto Retry;
    }

	for (i=0;i<MAX_STREAM_NUM;i++)
	{
		if(p->stream[i].extradata){
			vpc_mem_free(p->stream[i].extradata);
		}
        
        if (p->stream[i].inter_framebuf)
        {
            vpc_mem_free(p->stream[i].inter_framebuf);
            p->stream[i].inter_framebuf = 0;
        }
	}
    return 1;
}

//--------------------------plugin interface---------------------------

int plugin_open_avformat(source_layer*pl, player_parameter *pp,FILLSAMPLE fillsample, void *key)
{
	FFMPEG_demux *s = (FFMPEG_demux *)pl->plug_pri_data;
	if( !s ) return 0;
	s->fillsample = fillsample;
	s->fillkey	= key;
	strcpy(s->uri, pp->play_url);
	if(pp->start_pos)
	{
		s->request_seek_pos = pp->start_pos;
        
	}
	s->buffer_limit = 50000;
    

    reset_timeout(s);
    
    s->time_corrected = 0;
    
	return 1;
}

int plugin_set_notify_avformat(source_layer*pl,MSG_GATEWAY notify, void *key )
{
	FFMPEG_demux *s = (FFMPEG_demux *)pl->plug_pri_data;
	if( !s ) return 0;
	s->notify_routine = notify;
	s->key = key;
	return 1;
}

int plugin_start_avformat(source_layer*pl,int mspos)
{
	FFMPEG_demux *s = (FFMPEG_demux *)pl->plug_pri_data;
	if( !s ) return 0;
	if( mspos )
	{
		s->request_seek_pos = mspos;
        
	}
    
	if( s->running )
	{
		s->pause = 0;
	}
	else
	{
		s->running = 1;
		s->worker_handle = vpc_thread_create(&s->pb_thread_id,DemuxStreamThread,pl);
	}
	return 1;
}

int plugin_eof_avformat(source_layer*pl)
{
	FFMPEG_demux *s = (FFMPEG_demux *)pl->plug_pri_data;
	if( !s ) return 1;
	return 	s->eof;
}

int plugin_close_avformat(source_layer*pl)
{
   
	FFMPEG_demux *s = (FFMPEG_demux *)pl->plug_pri_data;
	if( !s ) return 0;
	s->running = 0;
	vpc_thread_destory(&s->worker_handle, s->pb_thread_id);
    s->time_corrected = 0;    
    av_parser_close(s->parser);
	vpc_mem_free( pl->plug_pri_data );
	pl->plug_pri_data = 0;    
	return 1;
}

int plugin_seek_avformat(source_layer*pl,int ms)
{
	FFMPEG_demux *s = (FFMPEG_demux *)pl->plug_pri_data;
	if( !s ) return 0;
	if(s->file_length)
	{
		if( ms > s->file_length ){
			return 0;
		}
		
        s->request_seek_pos = ms;
        
        
		s->pause = 0;
      
        return 1;
	}
	return 0;
}

int plugin_pause_avformat(source_layer*pl)
{
	FFMPEG_demux *s = (FFMPEG_demux *)pl->plug_pri_data;
	if( !s ) return 0;
	 s->pause = 1;
	return 1;
}

#endif //enable ffmpeg

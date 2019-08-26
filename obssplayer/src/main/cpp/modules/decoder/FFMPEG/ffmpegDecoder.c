#include "common.h"

#if ENABLE_FFMPEG

#define  inline __inline
#include "inttypes.h"
#include "common/FFMPEG_lib/include/libavcodec/avcodec.h"

#define MAX_FRAME_SIZE 128*1024//32*1024

typedef struct FFMpegDec{
	uint8_t *decBuffer;
	int		allBufSize;
	
	//ffmpeg variables
	AVCodecContext  *pCodecCtx;
	AVCodec         *pCodec;
	AVFrame         *pFrame; 
	AVPacket		pkt;
	timestamp_reorder reorder;
}FFMpegDec;

//--------------------------------video decoder-------------------------------

static int  MP10_open(codec_layer *codec,uint8_t *extra, int size);
static int32_t MP10_decode(codec_layer *codec, decode_context *dc);
static void MP10_close(codec_layer *codec);
static void MP10_flush(codec_layer *codec);

static void *MP10_create(const void *parm, int size)
{
	codec_layer *codec = (codec_layer *)vpc_mem_alloc(sizeof(codec_layer));
	if ( codec )
	{
		memset(codec, 0, (sizeof *codec));
		/* Set the function pointers */
		codec->open		= MP10_open;
		codec->decode	= MP10_decode;
		codec->close	= MP10_close;
		codec->flush    = MP10_flush;
	}
	return codec;
}

static int32_t MP10_available(const long parm)
{
	if( parm != H264_ID )
	{
		return 0;
	}
	return 1;
}

static void MP10_GetVersion(char buffer[VERSION_MAX_LENGTH])
{
	sprintf(buffer, "Video Decoder:%d", 95);
}

static void MP10_release(void *codec)
{
	vpc_mem_free(codec);
}

static const node_boot_func H264_Decoder =
{
	"FFMPEG 3.3.9",
	"H264",
	CODEC_ID, 
	H264_ID,
	PRIORITY_STANDARD,
	MP10_GetVersion,
	MP10_available,
	MP10_create,
	MP10_release,
};

/*some private data , Don't directly access them*/
static int  MP10_open(codec_layer *codec,uint8_t *extra, int size)
{
	AVCodecContext  *ctx;
	AVCodec   *c;
	FFMpegDec *dt = (FFMpegDec*)vpc_mem_alloc(sizeof(FFMpegDec));
	AVDictionary* options = NULL;
	if(!dt) return 0;
	avcodec_register_all();
// 	dt->decBuffer = (uint8_t*)vpc_mem_alloc(MAX_FRAME_SIZE);
// 	dt->allBufSize = MAX_FRAME_SIZE;
	codec->plug_pri_data = (void*)dt;
	
	dt->pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if(!dt->pCodec){
		return 0; // Codec not found
	}
	dt->pFrame	= av_frame_alloc();
	if( !dt->pFrame){
		return 0;
	}
	dt->pCodecCtx = avcodec_alloc_context3(dt->pCodec);
	if(!dt->pCodecCtx){
		return 0;
	}
    if( size > 0 )
    {
        dt->pCodecCtx->extradata_size =  size;
        dt->pCodecCtx->extradata = av_malloc( (size+15)&(~15));
        memcpy(dt->pCodecCtx->extradata, extra , size);
    }
	else{ // H264 is avc format ES
		av_dict_set(&options,  "is_avc" ,  "1" , 0);
		av_dict_set(&options,  "nal_length_size" ,  "4" , 0);
	}

	dt->pCodecCtx->thread_count = 4;
	c = dt->pCodec;
	ctx = dt->pCodecCtx;
	ctx->codec_id = c->id;

	if (avcodec_open2(ctx, c, &options) < 0) {
		//can't open decoder
		return 0;
	}
	video_flush_ts(&dt->reorder);
	return 1;
}

/*
static int processH264(FFMpegDec*dt, unsigned char *buf, int size)
{
	uint32_t h = ~0;
	int i = 0;
	char szhead[4]={0,0,0,1};
	int output_size = 0,nal_size = 0,over_size = size + 256;
	unsigned char *p = buf, *outp = dt->decBuffer;

	if( dt->allBufSize < over_size ){
		vpc_mem_free(dt->decBuffer);
		dt->decBuffer = vpc_mem_alloc((over_size+255)&(~255));
		dt->allBufSize = (over_size+255)&(~255);
        outp = dt->decBuffer;
	}

	for (i=0;i<4;i++)
	{
		h = (h<<(i*8))|buf[i];
		if((h&0xFFFFFF)==1)
		{
			memcpy(outp,buf,size);
			return size;
		}
	}

	while (size>0)
	{
		nal_size = p[0]<<8;
		nal_size |= p[1];

		nal_size -= 2;
		if(nal_size>size){
			return 0;
		}

		memcpy(outp,szhead,4);
		outp += 4;
		output_size +=4;
		memcpy(outp,p+2,nal_size);
		outp += nal_size;
		output_size += nal_size;

		p += nal_size+2;
		size -= nal_size+2;
	}
	return output_size;
}
*/

static int32_t MP10_decode(codec_layer *codec, decode_context *dc)
{
	FFMpegDec	*dt = (FFMpegDec*)codec->plug_pri_data;
	int got_picture = 0;
	int consumed = 0;
	AVPacket pkt;
	
	av_init_packet(&pkt);

	if( dc->in_stream )
	{
		//change h264 stream into 0x00 00 00 01
		pkt.size = dc->in_len;//processH264(dt,dc->in_stream, dc->in_len);
		pkt.data = dc->in_stream;//dt->decBuffer;
		pkt.pts  = dc->in_ts;		
		video_insert_ts(&dt->reorder,dc->in_ts);
	}
	else
	{
		pkt.size = 0;
		pkt.data = 0;
	}

// 	{
// 		static FILE *fp = 0;
// 		if(!fp)
// 			fp = fopen("d:\\b.264","wb");
// 		if(fp){
// 			int size = pkt.size;
// 			unsigned char *p = (char*)pkt.data;
// 			static char szhead[4] = {0,0,0,1};

// 			fwrite(p,size,1,fp);
// 			fflush(fp);

// 			while (size>0)
// 			{
// 				int nal_size = p[0]<<24;
// 				nal_size |= p[1]<<16;
// 				nal_size |= p[2]<<8;
// 				nal_size |= p[3];
// 
// 				fwrite(szhead,4,1,fp);
// 				fwrite(p+4,nal_size,1,fp);
// 				fflush(fp);
// 
// 				nal_size += 4;
// 
// 				p += nal_size;
// 				size -= nal_size;
// 			}
// 		}
//	}
	//printf("enter decoder a frame %d\r\n",dc->in_ts);
	//start decode
	consumed = avcodec_decode_video2(dt->pCodecCtx, dt->pFrame, &got_picture,&pkt);
	if( consumed == -1 ){
		return -2; //codec error
	}
	else if( !consumed && !got_picture )
		return -1;//end of stream


	if (got_picture)
	{
		dc->out[0] = dt->pFrame->data[0];
		dc->out[1] = dt->pFrame->data[1];
		dc->out[2] = dt->pFrame->data[2];
		dc->stride[0] = dt->pFrame->linesize[0];
		dc->stride[1] = dt->pFrame->linesize[1];
		dc->stride[2] = dt->pFrame->linesize[2];
		dc->height = dt->pFrame->height;
		dc->width  = dt->pFrame->width;
		dc->out_ts = video_get_ts(&dt->reorder);
		//printf("get a frame %d\r\n",dc->out_ts);

// 		{
// 			static FILE *out_file = 0;
// 			int i;
// 			if(!out_file)out_file = fopen("d:\\a.yuv","wb");
// 
// 			if(dc->out[0])
// 			{
// 
// 				for(i = 0; i < dc->height; i++)
// 				{
// 					fwrite(dc->out[0] + dc->stride[0] * i, 1, dc->width, out_file);
// 				}
// 				for(i = 0; i < dc->height / 2; i++)
// 				{
// 					fwrite(dc->out[1] + dc->stride[1] * i, 1, dc->width / 2, out_file);
// 				}
// 				for(i = 0; i < dc->height / 2; i++)
// 				{
// 					fwrite(dc->out[2] + dc->stride[2] * i, 1, dc->width / 2, out_file);
// 				}
// 				fflush(out_file);
// 			}
// 		}
	}
	return dc->in_len;
}

static void MP10_close(codec_layer *codec)
{
	FFMpegDec*dt = (FFMpegDec*)codec->plug_pri_data;
	if(dt)
	{
		if (dt->pCodecCtx)
		{
            if (dt->pCodecCtx->extradata)
            {
                av_free(dt->pCodecCtx->extradata);
				dt->pCodecCtx->extradata = 0;
            }
			avcodec_free_context(&dt->pCodecCtx);
			av_frame_free(&dt->pFrame);
		}

		if(dt->decBuffer){
			vpc_mem_free(dt->decBuffer);
		}
		vpc_mem_free(dt);
	}
	codec->plug_pri_data = 0;
}

void MP10_flush(codec_layer *codec)
{
	FFMpegDec *dt = (FFMpegDec*)codec->plug_pri_data;
	if(dt)
	{
		avcodec_flush_buffers(dt->pCodecCtx);
		video_flush_ts(&dt->reorder);
	}
}

//--------------------------------audio decoder-------------------------------

typedef struct FFMpegAudioDec{
	//ffmpeg variables
	AVCodecContext  *pCodecCtx;
	AVCodec         *pCodec;
	AVFrame         *pFrame; 
	AVPacket		pkt;
	/* audio convert */
// 	struct AudioParams audio_tgt;
// 	struct SwrContext *swr_ctx;

}FFMpegAudioDec;


static int  FFMpeg_audio_open(codec_layer *codec,uint8_t *extra, int size);
static int32_t FFMpeg_audio_decode(codec_layer *codec, decode_context *dc);
static void FFMpeg_audio_close(codec_layer *codec);
static void FFMpeg_audio_flush(codec_layer *codec);

static void *FFMpeg_audio_create(const void *parm, int size)
{
	codec_layer *codec = (codec_layer *)vpc_mem_alloc(sizeof(codec_layer));
	if ( codec )
	{
		memset(codec, 0, (sizeof *codec));
		/* Set the function pointers */
		codec->open		= FFMpeg_audio_open;
		codec->decode	= FFMpeg_audio_decode;
		codec->close	= FFMpeg_audio_close;
		codec->flush    = FFMpeg_audio_flush;
	}
	return codec;
}

static int32_t FFMpeg_audio_available(const long parm)
{
	if( parm != AAC_ID &&
		parm != AACPP_ID &&
		parm != AACP_ID &&
		parm != MP3_ID)
	{
		return 0;
	}
	return 1;
}

static void FFMpeg_audio_GetVersion(char buffer[VERSION_MAX_LENGTH])
{
	sprintf(buffer, "audio Decoder:%d", 95);
}

static void FFMpeg_audio_release(void *codec)
{
	vpc_mem_free(codec);
}

static const node_boot_func mp3_Decoder =
{
	"FFMPEG 3.3.9",
	"MP3",
	CODEC_ID, 
	MP3_ID,
	PRIORITY_STANDARD,
	FFMpeg_audio_GetVersion,
	FFMpeg_audio_available,
	FFMpeg_audio_create,
	FFMpeg_audio_release,
};

static const node_boot_func aac_LC_Decoder =
{
	"FFMPEG 3.3.9",
	"AAC",
	CODEC_ID, 
	AAC_ID,
	PRIORITY_STANDARD+1,
	FFMpeg_audio_GetVersion,
	FFMpeg_audio_available,
	FFMpeg_audio_create,
	FFMpeg_audio_release,
};

static const node_boot_func aac_HE_Decoder =
{
	"FFMPEG 3.3.9",
	"AAC HE",
	CODEC_ID, 
	AACP_ID,
	PRIORITY_STANDARD+1,
	FFMpeg_audio_GetVersion,
	FFMpeg_audio_available,
	FFMpeg_audio_create,
	FFMpeg_audio_release,
};

static const node_boot_func aac_HE_V2_Decoder =
{
	"FFMPEG 3.3.9",
	"AAC HE V2",
	CODEC_ID, 
	AACPP_ID,
	PRIORITY_STANDARD+1,
	FFMpeg_audio_GetVersion,
	FFMpeg_audio_available,
	FFMpeg_audio_create,
	FFMpeg_audio_release,
};

/*some private data , Don't directly access them*/
static int  FFMpeg_audio_open(codec_layer *codec,uint8_t *extra, int size)
{
	AVCodecContext  *ctx;
	AVCodec   *c;
	enum AVCodecID id_codec = AV_CODEC_ID_NONE;
	FFMpegAudioDec *dt = (FFMpegAudioDec*)vpc_mem_alloc(sizeof(FFMpegAudioDec));
	if(!dt) return 0;
	avcodec_register_all();
// 	dt->decBuffer = (uint8_t*)vpc_mem_alloc(MAX_FRAME_SIZE);
// 	dt->allBufSize = MAX_FRAME_SIZE;
	codec->plug_pri_data = (void*)dt;

	switch (codec->codecid)
	{
	case AAC_ID:
	case AACP_ID:
	case AACPP_ID:
		id_codec = AV_CODEC_ID_AAC;
		break;
	case MP3_ID:
		id_codec = AV_CODEC_ID_MP3;
		break;
	default:
		break;
	}
	dt->pCodec = avcodec_find_decoder( id_codec );
	if(!dt->pCodec){
		return 0; // Codec not found
	}
	dt->pFrame	= av_frame_alloc();
	if( !dt->pFrame){
		return 0;
	}
	dt->pCodecCtx = avcodec_alloc_context3(dt->pCodec);
	if(!dt->pCodecCtx){
		return 0;
	}
    if( size > 0 )
    {
		dt->pCodecCtx->extradata_size =  size;
		dt->pCodecCtx->extradata = vpc_mem_alloc( size+16);
		memcpy(dt->pCodecCtx->extradata, extra , size);
    }

	c = dt->pCodec;
	ctx = dt->pCodecCtx;
	ctx->codec_id = c->id;
	if (avcodec_open2(ctx, c,0) < 0) {
		return 0;
	}
	return 1;
}

static void aout_Interleave_float( void *dst, const void *const *srcv, unsigned samples, unsigned chans)
{
	do {
		int i,j,k;
		int16_t *d = (int16_t *)dst;
		for( i = 0; i < chans; i++ ) {
			const float *s = (const float *)srcv[i];
			for( j = 0, k = 0; j < samples; j++, k += chans )
			{
				double gainSample = *s++;
				gainSample = (gainSample < -1.0) ? -1.0 : (gainSample > 1.0) ? 1.0 : gainSample;
				d[k] = (int16_t)(gainSample * 32767.f);
			}
			d++;
		}
	} while(0);
}

static void aout_Interleave_int16( void *dst, const void *const *srcv, unsigned samples, unsigned chans)
{
	do {
		int i,j,k;
		int16_t *d = (int16_t *)dst;
		for( i = 0; i < chans; i++ ) {
			const int16_t *s = (const int16_t *)srcv[i];
			for( j = 0, k = 0; j < samples; j++, k += chans )
				d[k] = *(s++);
			d++;
		}
	} while(0);
}

static int32_t FFMpeg_audio_decode(codec_layer *codec, decode_context *dc)
{
	FFMpegAudioDec	*dt = (FFMpegAudioDec*)codec->plug_pri_data;
	int got_frame = 0;
	int consumed = 0;
	AVPacket pkt;
	int i;
	
	av_init_packet(&pkt);

	if( dc->in_stream )
	{
// 		if(1 )
// 		{
// 			static FILE *fp = 0;
// 			if(!fp)
// 			{
// 				fp = fopen("d:\\ff_aac.bin","wb");
// 			}
// 			else
// 			{
// 				fwrite(dc->in_stream, dc->in_len,1, fp);
// 				fflush(fp);
// 				if(ftell(fp)>1024*512)
// 				{
// 					fclose(fp);
// 					fp = 0;
// 				}
// 			}
// 		}

		/* 2 bytes is BE length */
		pkt.size = dc->in_len - 2;
		pkt.data = dc->in_stream + 2;
	}
	else
	{
		pkt.size = 0;
		pkt.data = 0;
	}

	//start decode
	consumed = avcodec_decode_audio4( dt->pCodecCtx, dt->pFrame, &got_frame,&pkt );
	if( consumed == -1 ){
		return -2; //codec error
	}
	else if( !consumed && !got_frame )
		return -1;//end of stream

	if (got_frame)
	{
		int channels = dt->pCodecCtx->channels;
		
		/* only support stereo */
		if( channels > 2)
			channels = 2;
		if( av_sample_fmt_is_planar(dt->pFrame->format) ){
			enum AVSampleFormat sample_fmt = AV_SAMPLE_FMT_NONE;
			const void *src[8] = {0};
			//interleave it
			for( i = 0; i < channels; i++)
				src[i] = dt->pFrame->extended_data[i];

			sample_fmt = av_get_planar_sample_fmt(dt->pFrame->format);
			if( sample_fmt == AV_SAMPLE_FMT_FLTP)
				aout_Interleave_float( dc->out[0], src, dt->pFrame->nb_samples, channels);
			else
				aout_Interleave_int16( dc->out[0], src, dt->pFrame->nb_samples, channels);
			dc->out_size = dt->pFrame->nb_samples*channels*2;
		}
		else{
			dc->out[0] = dt->pFrame->data[0];
			dc->out_size = av_samples_get_buffer_size(NULL, channels,dt->pFrame->nb_samples, dt->pFrame->format, 1);
		}
	}
	return consumed + 2; // 2 byets length
}

static void FFMpeg_audio_close(codec_layer *codec)
{
	FFMpegAudioDec*dt = (FFMpegAudioDec*)codec->plug_pri_data;
	if(dt)
	{
		if (dt->pCodecCtx)
		{
			if (dt->pCodecCtx->extradata)
			{
				vpc_mem_free(dt->pCodecCtx->extradata);

                dt->pCodecCtx->extradata = 0;
            }
			avcodec_free_context(&dt->pCodecCtx);
			av_frame_free(&dt->pFrame);
		}
		vpc_mem_free(dt);
	}
	codec->plug_pri_data = 0;
}

void FFMpeg_audio_flush(codec_layer *codec)
{
	FFMpegAudioDec *dt = (FFMpegAudioDec*)codec->plug_pri_data;
	if(dt)
	{
		avcodec_flush_buffers(dt->pCodecCtx);
	}
}

/*
#ifdef __VPC_DLL__
DYNAMIC_EXPORT void vpc_register_module(void *pc, void* pluginHandle, REGISTERTOOLS regfunc, int version)
{
	if( version<=0x80)
	{
		if( regfunc )
		{
			regfunc(pc, &H264_Decoder, pluginHandle );
			regfunc(pc, &AAC_Decoder, pluginHandle );
		}
	}
}
#else*/
void ffmpeg_h264_register(void *pc )
{
	RegisterTools(pc, &H264_Decoder,0);
	RegisterTools(pc, &mp3_Decoder,0);
// 	RegisterTools(pc, &aac_LC_Decoder,0);
// 	RegisterTools(pc, &aac_HE_Decoder,0);
// 	RegisterTools(pc, &aac_HE_V2_Decoder,0);
}
//#endif
#endif //enable ffmpeg

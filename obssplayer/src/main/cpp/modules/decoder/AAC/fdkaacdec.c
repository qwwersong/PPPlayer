
#include "common.h"

#ifdef _MSC_VER
#define inline __inline
#endif

#include "./libFdk_aac/include/aacenc_lib.h"
#include "./libFdk_aac/include/aacdecoder_lib.h"
#include <stdio.h>

// #include "../../../core/android/logger.h"
// #define LOG_TAG "fdkaac"

typedef struct fdk_Conetext
{
	HANDLE_AACDECODER decHandle;
	int	lengthPerFrame;
	AACENC_InfoStruct infoStruct;
}fdk_Conetext;

static int  fdkaac_open(codec_layer *codec,uint8_t *parm, int size);
static int32_t fdkaac_decode(codec_layer *codec, decode_context *dc);
static void fdkaac_close(codec_layer *codec);

static int32_t fdkaac_available(const long parm)
{
	if(parm != AAC_ID && parm != AACP_ID && parm != AACPP_ID)
	{
		return 0;
	}
	return 1;
}

static codec_layer *fdkaac_create(const void *parm, int size)
{
	codec_layer *codec;
	/* Initialize all variables that we clean on shutdown */
	codec = (codec_layer *)vpc_mem_alloc(sizeof(codec_layer));
	if (codec)
	{
		/* Set the function pointers */
		codec->open		= fdkaac_open;
		codec->decode	= fdkaac_decode;
		codec->close	= fdkaac_close;
	}
	return codec;
}

static void fdkaac_release(void *p)
{
	vpc_mem_free(p);
}

static const node_boot_func fdkAAC = {
	"fdk-AAC",
	"AAC",
	CODEC_ID, 
	AAC_ID,
	PRIORITY_STANDARD+200,
	NULL,
	fdkaac_available,
	fdkaac_create,
	fdkaac_release
};

static const node_boot_func fdkAAC_HE = {
	"fdk-AAC",
	"AAC",
	CODEC_ID, 
	AACP_ID,
	PRIORITY_STANDARD + 200,
	NULL,
	fdkaac_available,
	fdkaac_create,
	fdkaac_release
};

static const node_boot_func fdkAAC_HE_V2 = {
	"fdk-AAC",
	"AAC",
	CODEC_ID, 
	AACPP_ID,
	PRIORITY_STANDARD + 200,
	NULL,
	fdkaac_available,
	fdkaac_create,
	fdkaac_release
};

#ifdef __VPC_DLL__
DYNAMIC_EXPORT void vpc_register_module(void *pc, void* pluginHandle, REGISTERTOOLS regfunc, int version)
{
	if( version<=0x80)
	{
		if( regfunc )
		{
			regfunc(pc, &fdkAAC, pluginHandle );
			regfunc(pc, &fdkAAC_HE, pluginHandle );
			regfunc(pc, &fdkAAC_HE_V2, pluginHandle );
		}
	}
}
#else
void aac_register(void *pc)
{
	RegisterTools(pc,&fdkAAC,0);
	RegisterTools(pc,&fdkAAC_HE,0);
	RegisterTools(pc,&fdkAAC_HE_V2,0);
}
#endif

int getAudioSpecificConfig( AACENC_InfoStruct *info,int sampleRate, int channels, int sbr, int ps)
{
	int a = 0;
	CHANNEL_MODE		mode;
	HANDLE_AACENCODER	handle;
	int aot = 2;
	switch (channels) {
	case 1: mode = MODE_1;       break;
	case 2: mode = MODE_2;       break;
	case 3: mode = MODE_1_2;     break;
	case 4: mode = MODE_1_2_1;   break;
	case 5: mode = MODE_1_2_2;   break;
	case 6: mode = MODE_1_2_2_1; break;
	default:
		return 0;
	}

	if (aacEncOpen(&handle, 0, channels) != AACENC_OK) {
		printf("Unable to open encoder\n");
		goto errExit;
	}
	
	/*
	fprintf(stderr, "Supported AOTs:\n");
	fprintf(stderr, "\t2\tAAC-LC\n");
	fprintf(stderr, "\t5\tHE-AAC\n");
	fprintf(stderr, "\t29\tHE-AAC v2\n");
	fprintf(stderr, "\t23\tAAC-LD\n");
	fprintf(stderr, "\t39\tAAC-ELD\n");
		*/

	//AOT 2-LC, 5 HE ,29 HE v2
	if( sbr ){
		if( ps)
			aot = 29;
		else
			aot = 5;
	}
	
	if (aacEncoder_SetParam(handle, AACENC_AOT, aot) != AACENC_OK) {
		fprintf(stderr, "Unable to set the AOT\n");
		goto errExit;
	}

	if (aacEncoder_SetParam(handle, AACENC_SAMPLERATE, sampleRate) != AACENC_OK) {
		fprintf(stderr, "Unable to set the AOT\n");
		goto errExit;
	}
	if (aacEncoder_SetParam(handle, AACENC_CHANNELMODE, mode) != AACENC_OK) {
		fprintf(stderr, "Unable to set the channel mode\n");
		goto errExit;
	}

	if (aacEncoder_SetParam(handle, AACENC_CHANNELORDER, 1) != AACENC_OK) {
		fprintf(stderr, "Unable to set the wav channel order\n");
		goto errExit;
	}

	if (aacEncoder_SetParam(handle, AACENC_TRANSMUX, 0 ) != AACENC_OK) {
		fprintf(stderr, "Unable to set the ADTS transmux\n");
		goto errExit;
	}

	if ( (a = aacEncEncode(handle, NULL, NULL, NULL, NULL)) != AACENC_OK) {
		fprintf(stderr, "Unable to initialize the encoder, err=%d\n", a);
		goto errExit;
	}

	if (aacEncInfo(handle, info) != AACENC_OK) {
		fprintf(stderr, "Unable to get the encoder info\n");
		goto errExit;
	}

errExit:
	if( handle ){
		aacEncClose(&handle);
	}

	if( info->confSize )
		return 1;

	return 0;
}

int  fdkaac_open(codec_layer *codec,uint8_t *extra, int size)
{
	char *audip_parm = (char*)extra;
	UCHAR *conf[1];  
	int sampleRate;
	int nChannels;
	int sbr_flag;
	int ps_flag;

	char *p;

	fdk_Conetext *app_context = vpc_mem_alloc(sizeof(fdk_Conetext));
	if( !app_context)
		return 0;

	/*freq:ch:sbr:bitrate*/
	p = audip_parm;
	sscanf(p,"%d/%d/%d/%d",	&sampleRate,&nChannels, &sbr_flag,&ps_flag);

	if( sampleRate <= 0 ) return 0;
	if( nChannels <= 0 ) return 0;
	if( nChannels>2) return 0;

	if( !getAudioSpecificConfig( &app_context->infoStruct, sampleRate, nChannels, sbr_flag,ps_flag ) )
		return 0;

	codec->plug_pri_data = app_context;
	app_context->decHandle = aacDecoder_Open(TT_MP4_RAW,1);
	if ( ! app_context->decHandle ){
		return 0;
	}

	conf[0] = app_context->infoStruct.confBuf;
	if (aacDecoder_ConfigRaw(app_context->decHandle, conf, &app_context->infoStruct.confSize) != AAC_DEC_OK) {
		return 0;
	}
	app_context->lengthPerFrame = app_context->infoStruct.frameLength*app_context->infoStruct.inputChannels;
 	return 1;
}

int32_t fdkaac_decode(codec_layer *codec, decode_context *dc)
{
	fdk_Conetext *app_context = (fdk_Conetext *)codec->plug_pri_data;
	int consumed = 0;
	int ret = 0;
	UINT pkt_size = dc->in_len-2;
	UINT valid_size = pkt_size;
	UCHAR *input_buf = dc->in_stream+2;

	static int fdk_flags = 0;

	//LOG_INFO("enter decoder =%d\r\n",pkt_size[0] );

    /* step 1 -> fill aac_data_buf to decoder's internal buf */
    ret = aacDecoder_Fill(app_context->decHandle, &input_buf, &pkt_size, &valid_size);
    if (ret != AAC_DEC_OK) {
        fprintf(stderr, "Fill failed: %x\n", ret);
		return 0;
    }
	consumed = dc->in_len - 2 - valid_size;

// 	if(1 )
// 	{
// 		static FILE *fp = 0;
// 		if(!fp)
// 		{
// 			fp = fopen("d:\\fdk_aac.bin","wb");
// 		}
// 		else
// 		{
// 			fwrite(dc->in_stream, dc->in_len,1, fp);
// 			fflush(fp);
// 			if(ftell(fp)>1024*512)
// 			{
// 				fclose(fp);
// 				fp = 0;
// 			}
// 		}
// 	}


    /* step 2 -> call decoder function */
	ret = aacDecoder_DecodeFrame( app_context->decHandle, (INT_PCM*)dc->out[0], app_context->lengthPerFrame, fdk_flags);
	if (ret == AAC_DEC_NOT_ENOUGH_BITS) {
        fprintf(stderr, "not enough\n");
        dc->out_size  = 0;
        /*
         * TODO FIXME
         * if not enough, get more data
         *
         */
    }
	if (ret != AAC_DEC_OK) {
		fprintf(stderr, "aacDecoder_DecodeFrame : 0x%x\n", ret);
		dc->out_size  = 0;
	}
	dc->out_size = app_context->lengthPerFrame*sizeof(INT_PCM);
// 	{
// 		static FILE *out_file = 0;
// 		int i;
// 		if(!out_file)out_file = fopen("d:\\a.pcm","wb");
// 
// 		if(pcm_buf_out)
// 		{
// 
// 			fwrite(pcm_buf_out, 1, outbuf_len_in_bytes, out_file);
// 			fflush(out_file);
// 		}
// 	}

//	OutputDebugString("acc_frame_over\r\n");
	return consumed+2;
}

void fdkaac_close(codec_layer *codec)
{
	fdk_Conetext *app_context = 0;
	app_context = (fdk_Conetext *)codec->plug_pri_data;
	if( app_context )
	{
		if(app_context->decHandle) aacDecoder_Close( app_context->decHandle);
	// 	if(app_context->buffer) vpc_mem_free(app_context->buffer);
		vpc_mem_free(app_context);
	}
	codec->plug_pri_data=0;
}






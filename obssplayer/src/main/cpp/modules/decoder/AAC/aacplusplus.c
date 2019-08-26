
#include "common.h"
#include "./aacpp/include/tma_aacPlusDec.h"

typedef struct _TIAC_APP_Conetext
{
	TMA_aacPlusDecHandle decHandle;
	aacPlusDec_param_t aac_par;
// 	uint8_t	*buffer;
// 	int alloc_len;
// 	int	real_len;
}TIAC_APP_Conetext;


/*
 *	AAC_PLUSPLUS
 */


static int  aacpp_open(codec_layer *codec,uint8_t *parm, int size);
static int32_t aacpp_decode(codec_layer *codec, decode_context *dc);
static void aacpp_close(codec_layer *codec);

static int32_t aacpp_available(const int parm)
{
	if(parm != AAC_ID && parm != AACP_ID)
	{
		return 0;
	}
	return 1;
}

static codec_layer *aacpp_create(void *parm, int size)
{
	codec_layer *codec;
	/* Initialize all variables that we clean on shutdown */
	codec = (codec_layer *)vpc_mem_alloc(sizeof(codec_layer));
	if (codec)
	{
		/* Set the function pointers */
		codec->open		= aacpp_open;
		codec->decode	= aacpp_decode;
		codec->close	= aacpp_close;
	}
	return codec;
}

static void aacpp_release(void *p)
{
	vpc_mem_free(p);
}

static const node_boot_func AAC_PLUSPLUS = {
	"AACPlusPlus",
	"AAC++",
	CODEC_ID, 
	AAC_ID,
	PRIORITY_STANDARD,
	NULL,
	aacpp_available,
	aacpp_create,
	aacpp_release
};

#ifdef __VPC_DLL__
DYNAMIC_EXPORT void vpc_register_module(void *pc, void* pluginHandle, REGISTERTOOLS regfunc, int version)
{
	if( version<=0x80)
	{
		if( regfunc )
		{
			regfunc(pc, &AAC_PLUSPLUS, pluginHandle );
		}
	}
}
#else
void aac_register(void *pc)
{
	RegisterTools(pc,&AAC_PLUSPLUS,0);
}
#endif


int  aacpp_open(codec_layer *codec,uint8_t *extra, int size)
{
	int err_code;
	char *audip_parm = (char*)extra;
	//	TeAudioHeader_t teAudioHdr;

	/*
	int sampleRate;
	int nChannels;
	int sbr_flag;
	int bitrate;
	int frame_size;  // in samples
	*/

	char *p;

	TIAC_APP_Conetext *app_context = vpc_mem_alloc(sizeof(TIAC_APP_Conetext));
	if( !app_context) return 0;

	/*
	from parm get aacPlusDec_param_t info;
	*/
	/*freq:ch:sbr:bitrate*/
	p = audip_parm;
	sscanf(p,"%d/%d/%d/%d",	&app_context->aac_par.sampleRate,&app_context->aac_par.channels,
							&app_context->aac_par.sbr_flag,&app_context->aac_par.ps_flag);

	/*it's a constant*/
// 	app_context->aac_par.ps_flag = 0;

	if( app_context->aac_par.sampleRate <= 0 ) return 0;
	if( app_context->aac_par.channels <= 0 ) return 0;

	if(app_context->aac_par.channels>2) return 0;



	//	if ( TeAudioHeader_buffer_verify(app_context->teAudioHdrbuffer) != TEAUD_HDR_ERR_NO_ERROR) {
	//	Trace(" TeAudioHeader_buffer_verify() failed\n");
	//		return 0;
	//	}

	//	TeAudioHeader_decode(app_context->teAudioHdrbuffer, &teAudioHdr);

	/*
	sampleRate = tma_get_sample_rate( teAudioHdr.sampleRateIndex); // in Hz
	nChannels  = teAudioHdr.channels;
	bitrate    = teAudioHdr.bitrate;
	frame_size = teAudioHdr.sbr_flag ? 2048 : 1024;
	if ( teAudioHdr.audioType != 1)
	{
	//	Trace("\n unsupported audio encoding type\n");
	return 0;
	}
	*/
	codec->plug_pri_data = app_context;
	app_context->decHandle = TMA_aacPlusDec_open(&app_context->aac_par, &err_code);
	if ( ! app_context->decHandle )
	{
	//	Trace("\n TMA_aacPlusDec_open() failed, error code = %d\n", err_code);
		return 0;
	}
 	return 1;
}

int32_t aacpp_decode(codec_layer *codec, decode_context *dc)
{
	TIAC_APP_Conetext *app_context = (TIAC_APP_Conetext *)codec->plug_pri_data;
	int err_code;
	const void *pcm_buf_out;
	int outbuf_len_in_bytes;
	int consumed = 0;

// 	short encframe = adc->in_len;
// 	char t = 0;
// 	t = encframe>>8;
// 	t = (char)encframe;
// 	if(app_context->alloc_len < dc->in_len )
// 	{
// 		if(app_context->buffer) vpc_mem_free(app_context->buffer);
// 		app_context->buffer = vpc_mem_alloc(dc->in_len);
// 		if(!app_context->buffer) return dc->in_len;
// 		app_context->alloc_len = dc->in_len;
// 	}
// 
// 	memcpy(app_context->buffer, dc->in_stream, dc->in_len );
// 	app_context->real_len = dc->in_len;
//	return adc->in_len;

/*
	{
		static FILE *audio_stamp = 0;
		unsigned char *p = (unsigned char *)&adc->in_len;
		TCHAR szError[32];
		static int time=0;
		if(!audio_stamp) audio_stamp = fopen("audio_stamp_ce_dec.aac","wb");
		fwrite(p+1,1, 1, audio_stamp );
		fwrite(p,1, 1, audio_stamp );
		fwrite(adc->in_stream,1, adc->in_len, audio_stamp );
		swprintf(szError,TEXT("t=%d,l=%d\r\n"), time++, adc->in_len );
//		OutputDebugString(szError);
	}
*/

	/*the first 2bytes is length,so skip it*/
	pcm_buf_out = TMA_aacPlusDec_oneFrame(
		app_context->decHandle, // aacPlusDecHandle handle,      // (i/o)
		dc->in_stream+2,          // const void      *inpBuf,      // (i), encoded frame data, excluding 2 bytes encFrameLen
		dc->in_len-2,				// unsigned short  encFrameLen,  // (i), the length of encoded frame data in bytes
		&consumed,				// consumed bytes
		&outbuf_len_in_bytes	// unsigned int	*outbuf_len_in_bytes // (o), the length of output buffer in bytes
		);
	if ( !pcm_buf_out) { // decoding failed
		err_code = TMA_aacPlusDec_errorCode(app_context->decHandle);
//		Trace("\n frame #%d decoding failed, err code = %d\n", frame_counter, err_code);
		return -1;
	}

	dc->out_size =	outbuf_len_in_bytes;
	dc->out[0] = (void *)pcm_buf_out;

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

void aacpp_close(codec_layer *codec)
{
	TIAC_APP_Conetext *app_context = 0;
	app_context = (TIAC_APP_Conetext *)codec->plug_pri_data;
	if(app_context->decHandle)TMA_aacPlusDec_close( app_context->decHandle);
// 	if(app_context->buffer) vpc_mem_free(app_context->buffer);
	vpc_mem_free(app_context);
	codec->plug_pri_data=0;
}






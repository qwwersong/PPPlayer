
#include "common.h"
#include "libamrnb/interf_dec.h"

/*
 *	amrnb
 */

static int  awrnb_open(codec_layer *codec ,uint8_t *parm, int size);
static int32_t awrnb_decode(codec_layer *codec, decode_context *dc);
static void awrnb_close(codec_layer *codec);

static const unsigned char block_size_nb[16]={ 13, 14, 16, 18, 20, 21, 27, 32, 6 , 0 , 0 , 0 , 0 , 0 , 0 , 1};

static void *awrnb_create(const void *parm, int size)
{
	codec_layer *codec = (codec_layer *)tmpc_mem_alloc(sizeof(codec_layer));
	if (codec)
	{
		memset(codec, 0, (sizeof *codec));
		/* Set the function pointers */
		codec->open		= awrnb_open;
		codec->decode	= awrnb_decode;
		codec->close	= awrnb_close;
	}
	return codec;
}

static int  awrnb_open(codec_layer *codec, uint8_t *parm, int size)
{
	codec->plug_pri_data = Decoder_Interface_init();
	return 1;
}

static int32_t awrnb_available(const long parm)
{
	if(parm != AMRNB_ID )
	{
		return 0;
	}
	return 1;
}

static void  amrnb_release(void *p)
{
	tmpc_mem_free(p);
}

static const node_boot_func AMRNB = {
	"AMRNB",
	"AMRNB",
	CODEC_ID, 
	AMRNB_ID,
	PRIORITY_STANDARD,
	NULL,
	awrnb_available,
	awrnb_create,
	amrnb_release,
};


void amrnb_register(void *pc )
{
	RegisterTools(pc, &AMRNB,0);
}

int32_t awrnb_decode(codec_layer *codec, decode_context *dc)
{
	int mode = (unsigned short)((dc->in_stream[0] >> 3) & 0x0F);
	int len = block_size_nb[mode];
	Decoder_Interface_Decode(codec->plug_pri_data, dc->in_stream, (short*)dc->out[0], 0);
	dc->out_size = 320;
	return len;
}

void awrnb_close(codec_layer *codec)
{
	if(codec->plug_pri_data)
	{
		Decoder_Interface_exit(codec->plug_pri_data);
		codec->plug_pri_data = 0;
	}
}


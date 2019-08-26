
/*
 
 #include "common.h"
 #include "libamrwb/dec_if.h"

static int  amrwb_open(codec_layer *codec,uint8_t *parm, int size);
static int32_t amrwb_decode(codec_layer *codec, decode_context *dc);
static void amrwb_close(codec_layer *codec);

static const unsigned char block_size_wb[16]={ 18, 24, 33, 37, 41, 47, 51, 59, 61, 6, 6, 0, 0, 0, 1, 1};

static codec_layer *amrwb_create(void *parm, int size)
{
	codec_layer *codec = (codec_layer *)tmpc_mem_alloc(sizeof(codec_layer));
	if (codec)
	{
		memset(codec, 0, (sizeof *codec));
		codec->open		= amrwb_open;
		codec->decode	= amrwb_decode;
		codec->close	= amrwb_close;
	}
	return codec;
}

static int32_t amrwb_available(const int parm)
{
	if(parm != AMRWB_ID )
	{
		return 0;
	}
	return 1;
}

static void amrwb_release(void *p)
{
	tmpc_mem_free(p);
}

static const node_boot_func AMRWB_NORMAL = {
	"AMRWB",
	"AMRWB",
	CODEC_ID, 
	AMRWB_ID,
	PRIORITY_STANDARD,
	NULL,
	amrwb_available,
	amrwb_create,
	amrwb_release,
};


void amrwb_register(void *pc )
{
	RegisterTools(pc, &AMRWB_NORMAL,0);
}

typedef struct audDec
{
	void *h;
	uint8_t *mid_buffer;
	int mid_size;
}audDec;


int amrwb_open(codec_layer *codec,uint8_t *extra, int size)
{
	audDec  * e = (audDec*)tmpc_mem_alloc(sizeof(audDec));
	e->h = D_IF_init();
	e->mid_buffer = tmpc_mem_alloc(2048);
	e->mid_size = 2048;
	codec->plug_pri_data = e;
	
	return (e != NULL);
}

int32_t amrwb_decode(codec_layer *codec, decode_context *dc)
{
	audDec  * e = (audDec*)codec->plug_pri_data;
	int mode = (Word16t)((dc->in_stream[0] >> 3) & 0x0F);
	int len = block_size_wb[mode];
	if( len > e->mid_size)
	{
		tmpc_mem_free(e->mid_buffer);
		e->mid_buffer = tmpc_mem_alloc(len);
		e->mid_size = len;
	}
	
	memcpy( e->mid_buffer, dc->in_stream, len );
	D_IF_decode(e->h, e->mid_buffer, (short*)dc->out[0], 0);
	dc->out_size = 640;
	return len;
}

void amrwb_close(codec_layer *codec)
{
	audDec  * e = (audDec*)codec->plug_pri_data;
	if( codec->plug_pri_data )
	{
		if( e->h)
			D_IF_exit(e->h);
		tmpc_mem_free(e->mid_buffer);
		codec->plug_pri_data = 0;
	}
}

 */
#include "common.h"
#include "libamrwb/dec_if.h"


/*
 *	amrwb
 */
static int  amrwb_open(codec_layer *codec,uint8_t *parm, int size);
static int32_t amrwb_decode(codec_layer *codec, decode_context *dc);
static void amrwb_close(codec_layer *codec);

static const unsigned char block_size_wb[16]={ 18, 24, 33, 37, 41, 47, 51, 59, 61, 6, 6, 0, 0, 0, 1, 1};

static void *amrwb_create(const void *parm, int size)
{
	codec_layer *codec = (codec_layer *)tmpc_mem_alloc(sizeof(codec_layer));
	if (codec)
	{
		memset(codec, 0, (sizeof *codec));
		codec->open		= amrwb_open;
		codec->decode	= amrwb_decode;
		codec->close	= amrwb_close;
	}
	return codec;
}

static int32_t amrwb_available(const long parm)
{
	if(parm != AMRWB_ID )
	{
		return 0;
	}
	return 1;
}

static void amrwb_release(void *p)
{
	tmpc_mem_free(p);
}

static const node_boot_func AMRWB_NORMAL = {
	"AMRWB",
	"AMRWB",
	CODEC_ID, 
	AMRWB_ID,
	PRIORITY_STANDARD,
	NULL,
	amrwb_available,
	amrwb_create,
	amrwb_release,
};


void amrwb_register(void *pc )
{
	RegisterTools(pc, &AMRWB_NORMAL,0);
}

/*some private data , Don't directly access them*/

int amrwb_open(codec_layer *codec,uint8_t *extra, int size)
{
	codec->plug_pri_data = D_IF_init();
	return (codec->plug_pri_data != NULL);
}

int32_t amrwb_decode(codec_layer *codec, decode_context *dc)
{
	int mode = (Word16t)((dc->in_stream[0] >> 3) & 0x0F);
	int len = block_size_wb[mode];
	D_IF_decode(codec->plug_pri_data, dc->in_stream, (short*)dc->out[0], 0);
	dc->out_size = 640;
	return len;
}

void amrwb_close(codec_layer *codec)
{
	if( codec->plug_pri_data )
	{
		D_IF_exit(codec->plug_pri_data);
		codec->plug_pri_data = 0;
	}
}

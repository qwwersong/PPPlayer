#include "common.h"
#include "rmH265.h"

#ifndef __EXCLUDE_H265__
#define MAX_FRAME_SIZE 32*1024
#define DEBUG_HEVC 0
//extern FILE *g_dec_bin;

static int  HEVC_open(codec_layer *codec, uint8_t *extra, int size);
static int32_t HEVC_decode(codec_layer *codec, decode_context *dc);
static void HEVC_close(codec_layer *codec);
static void HEVC_flush(codec_layer *codec);

typedef struct VideoDec{
	RM_VIDEO_CODECAPI          rmhevcdecApis;
	RM_HANDLE                  hevcdec;
	RM_CODECBUFFER			 in_data;

	//bit stream memory
 	uint8_t *decBuffer;
	int		allBufSize;
	int		leftBufSize;
	
	int		flushed;
	timestamp_reorder reorder;
}VideoDec;

static codec_layer *HEVC_create(const void *parm, int size)
{
	codec_layer *codec = (codec_layer *)vpc_mem_alloc(sizeof(codec_layer));
	if ( codec )
	{
		memset(codec, 0, (sizeof *codec));
		/* Set the function pointers */
		codec->open		= HEVC_open;
		codec->decode	= HEVC_decode;
		codec->close	= HEVC_close;
		codec->flush	= HEVC_flush;
	}
	return codec;
}

static int32_t HEVC_available(const long parm)
{
	if( parm != H265_ID )
	{
		return 0;
	}
	return 1;
}

static void HEVC_GetVersion(char buffer[VERSION_MAX_LENGTH])
{
// 	sprintf(buffer, "Video Decoder:%d", GetDecoderVersion());
}

static void HEVC_free(codec_layer *codec)
{
	vpc_mem_free(codec);
}

static const node_boot_func H265Decoder =
{
	"H265",
	"H265",
	CODEC_ID, 
	H265_ID,
	PRIORITY_STANDARD,
	HEVC_GetVersion,
	HEVC_available,
	HEVC_create,
	HEVC_free,
};

void H265_register(void *pc )
{
	RegisterTools(pc, &H265Decoder,0);
}


int HEVC_open(codec_layer *codec, uint8_t *extra, int size)
{
	RM_S32	cpu_num = vpc_get_cpu_count()+1;
	RM_U32	nRet;
	VideoDec *dt = (VideoDec*)vpc_mem_alloc(sizeof(VideoDec));
	if(!dt) return 0;

	nRet = rmGetDecHandle(&dt->rmhevcdecApis);
	if(nRet != RM_RETURN_OK){
		return 0;
	}

	nRet = dt->rmhevcdecApis.Open(&dt->hevcdec);
	if(nRet != RM_RETURN_OK){
		return 0;
	}
	
	if( cpu_num < 2 ) 
		cpu_num = 2;
	
	nRet = dt->rmhevcdecApis.Set(dt->hevcdec, RM_PID_CPUNUM, (RM_PTR)&cpu_num);
	if(nRet != RM_RETURN_OK){
		return 0;
	}
 	dt->decBuffer = (uint8_t*)vpc_mem_alloc(MAX_FRAME_SIZE);
 	dt->allBufSize = MAX_FRAME_SIZE;
	dt->leftBufSize = 0;
	video_flush_ts(&dt->reorder);
	codec->plug_pri_data = (void*)dt;
	return 1;
}

// 					{
// 						static FILE *fpyuv = 0;
// 						if(!fpyuv)
// 						{
// 							fpyuv = fopen("d:\\yuv.yuv","wb");
// 						}
// 
// 						if(1)
// 						{
// 							unsigned char*buf;
// 							int i;
// 							int width = dt->m_Deccontext->width;
// 							int height = dt->m_Deccontext->height;
// 							int stride = dt->m_Deccontext->stride;
// 
// 							buf = dt->m_Deccontext->y;
// 							for(i = 0; i < height; i++)
// 							{
// 								fwrite(buf, 1, width, fpyuv);
// 								buf += stride;
// 							}
// 							buf = dt->m_Deccontext->u;
// 							for(i = 0; i < height / 2; i++)
// 							{
// 								fwrite(buf, 1, width / 2, fpyuv);
// 								buf += stride / 2;
// 							}
// 							buf = dt->m_Deccontext->v;
// 							for(i = 0; i < height / 2; i++)
// 							{
// 								fwrite(buf, 1, width / 2, fpyuv);
// 								buf += stride / 2;
// 							}
// 						}
// 
// 					}


int32_t HEVC_decode(codec_layer *codec,decode_context *dc)
{
	VideoDec	*dt = (VideoDec*)codec->plug_pri_data;
	RM_VIDEO_BUFFER         out_data={0};
	RM_VIDEO_BASICINFO    pStreamInfo={0};
	RM_U32	nRet;

#if DEBUG_HEVC
	static FILE *fp = 0;
	if(!fp)
		fp = fopen("d:\\out_ts.txt","w");
#endif   
    	do {
		do{
			nRet = dt->rmhevcdecApis.Process(dt->hevcdec, (RM_PTR*)&out_data, &pStreamInfo);
			if(nRet == RM_RETURN_OK){
				if(pStreamInfo.Format.Type != RM_VIDEO_FRAME_NULL)
				{
					dc->out[0] = out_data.Buffer[0];
					dc->out[1] = out_data.Buffer[1];
					dc->out[2] = out_data.Buffer[2];
					dc->stride[0] = out_data.Stride[0];
					dc->stride[1] = out_data.Stride[1];
					dc->stride[2] = out_data.Stride[2];
					//dc->out_ts = (TIME_TYPE)out_data.Time;
					dc->width  = pStreamInfo.Format.Width;
					dc->height = pStreamInfo.Format.Height;
					dc->out_ts = video_get_ts(&dt->reorder);
#if DEBUG_HEVC
					fprintf(fp,"--------------------out_ts=%d\r\n",dc->out_ts);
					fflush(fp);
					if( ftell(fp)>1024*32){
						fclose(fp);
						fp = 0;
					}
#endif
					goto GOT_FRAME;
				}
			}
			else if (nRet != RM_RETURN_INPUT_NO_ENOUGH)
			{
				dt->leftBufSize = 0;
				goto End;
			}
		}while(pStreamInfo.Flag);

	}while( nRet != RM_RETURN_INPUT_NO_ENOUGH);

	dt->leftBufSize = dt->in_data.Length - pStreamInfo.UsedLens;
	if( dt->leftBufSize>0 ){
		memmove(dt->decBuffer,dt->decBuffer+pStreamInfo.UsedLens,dt->leftBufSize);
	}
End:
	if( dc->in_len && (dt->allBufSize < dc->in_len + dt->leftBufSize) )
	{
		uint8_t *pold = dt->decBuffer;
		int size = dc->in_len + dt->leftBufSize;
		size = (size+255)&(~255);
		dt->decBuffer = vpc_mem_alloc(size);
		dt->allBufSize = size;		
		memcpy(dt->decBuffer,pold,dt->leftBufSize);
		vpc_mem_free(pold);
	}

	//maybe there are YUV in decoder cache!!!
	if(!dc->in_stream ) 
	{
		if( !dt->flushed )
		{
			//flush pending yuv
			RM_U32 flush = 1;
			dt->rmhevcdecApis.Set(dt->hevcdec, RM_PID_HEVCDEC_END_FLUSH, (RM_PTR*)&flush);
			dt->flushed = 1;
			return 0;
		}
		else
		{
			return -1;
		}
	}
	if(!dt->decBuffer) return 0;

	memcpy(dt->decBuffer+dt->leftBufSize,dc->in_stream, dc->in_len);
	dt->in_data.Buffer = dt->decBuffer;
	dt->in_data.Length = dc->in_len+dt->leftBufSize;
// 	dt->in_data.Time = dc->in_ts;

	video_insert_ts(&dt->reorder,dc->in_ts);

#if DEBUG_HEVC
	{
		if(((dt->in_data.Buffer[4] >> 1 )& 0x3f) == 32 ){
			fprintf(fp,"in_ts=%d-Iframe\n",dc->in_ts);
			fflush(fp);
		}
		else
		{
			fprintf(fp,"in_ts=%d\n",dc->in_ts);
			fflush(fp);
		}
		if( ftell(fp)>1024*32){
			fclose(fp);
			fp = 0;
		}
	}


	{
		static FILE * fp = 0;
		if( fp&& ftell(fp)>1024*1024*3 && (((dt->in_data.Buffer[4] >> 1 )& 0x3f) == 32)){
			fclose(fp);
			fp = 0;
		}
		if(!fp)
		{
			fp = fopen("d:\\23.bin","wb");
		}
		if(fp)
		{
			fwrite(dt->in_data.Buffer,1,dt->in_data.Length,fp);
			fflush(fp);
		}
	}
#endif
// 	if(((dt->in_data.Buffer[4] >> 1 )& 0x3f) == 32 ){
// 		dt->rmhevcdecApis.Set(dt->hevcdec, RM_PID_FLUSH, NULL);
// 	}
    
    ///
    /*
    {
        static FILE * fp = 0;
        if( fp&& ftell(fp)>1024*1024*3 && (((dt->in_data.Buffer[4] >> 1 )& 0x3f) == 32)){
            fclose(fp);
            fp = 0;
        }
        if(!fp)
        {
            fp = g_dec_bin;
        }
        if(fp)
        {
            fwrite(dt->in_data.Buffer,1,dt->in_data.Length,fp);
            fflush(fp);
        }
    }
    */
    ///
	dt->rmhevcdecApis.Set(dt->hevcdec, RM_PID_INPUTDATA, (RM_PTR)&dt->in_data);
	dc->out[0] = 0; //no frame
	return dc->in_len;

GOT_FRAME:
	return 0; //get a frame
}

void HEVC_flush(codec_layer *codec)
{
	VideoDec *dt = (VideoDec*)codec->plug_pri_data;
	if(dt)
	{
// 		RM_U32	nRet;
		dt->flushed =0;
		video_flush_ts(&dt->reorder);
		dt->rmhevcdecApis.Set(dt->hevcdec, RM_PID_FLUSH, NULL);
	}
}


void HEVC_close(codec_layer *codec)
{
	VideoDec *dt = (VideoDec*)codec->plug_pri_data;
	if(dt)
	{
		dt->rmhevcdecApis.Close(dt->hevcdec);
		dt->hevcdec = 0;
		if(dt->decBuffer)
		{
			vpc_mem_free(dt->decBuffer);
			dt->decBuffer = 0;
		}
		vpc_mem_free(dt);
	}
	codec->plug_pri_data = 0;
}
#endif


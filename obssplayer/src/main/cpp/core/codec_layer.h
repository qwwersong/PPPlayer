#ifndef __CODEC_LAYER_H__
#define __CODEC_LAYER_H__

#include <android/data_type.h>

#define CODEC_ID			FOURCC('C','O','D','E')

/*video codec*/
#define TIVC_ID				FOURCC('T','I','V','C')
#define TIVC7_ID			FOURCC('T','I','V','7')

/*audio codec*/
#define AAC_ID				FOURCC('A','A','C','_')
#define MP3_ID				FOURCC('M','P','3','_')
#define AACPP_ID			FOURCC('A','C','+','+')
#define AACP_ID				FOURCC('A','A','C','+')
#define AMRNB_ID			FOURCC('A','M','N','B')
#define AMRWB_ID			FOURCC('A','M','W','B')
#define AMRWB_PLUS_ID		FOURCC('A','W','B','P')
#define XVID_ID				FOURCC('X','V','I','D')
#define H264_ID				FOURCC('H','2','6','4')
#define H265_ID				FOURCC('H','E','V','C')
#define AVCODEC_ID			FOURCC('A','V','C','O')

typedef struct decode_context
{
	//in
	uint8_t *in_stream;
	int32_t in_len;
	TIME_TYPE in_ts;
	TIME_TYPE stream_time;
	//out
	uint8_t *out[3]; /*only out[0] for audio*/
	int	out_size;/*only for audio*/
	//only video
	int32_t width;
	int32_t height;
	int32_t stride[3];
	TIME_TYPE out_ts;
}decode_context;

typedef struct codec_layer 
{
	int  (*open)(struct codec_layer *codec, uint8_t *extradata, int extrsize);

	// return -1 stand for error.
	//other,consumed bytes
	int32_t (*decode)(struct codec_layer *codec, decode_context*dc );
	void (*close)(struct codec_layer *codec);
	void (*flush)(struct codec_layer *codec);
	//some private data of a codec
	int	codecid;
	void *plug_pri_data;
}codec_layer;

#endif//__CODEC_LAYER_H__

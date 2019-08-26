#ifndef __SOURCE_LAYER_H__
#define __SOURCE_LAYER_H__

#include <stdint.h>

typedef void (*MSG_GATEWAY) (void* key, int notify_id, long param1, long param2);
typedef int (*FILLSAMPLE) (void *ptx,void *sp);

typedef struct source_layer source_layer;

/*open*/
typedef int32_t (*OPEN)(source_layer *pl,player_parameter *pp, FILLSAMPLE fillsample, void *key);

/*begin to get data*/
typedef int32_t (*START)(source_layer *pl,int mspos);

/*End of File*/
typedef int32_t (*ISEOF)(source_layer *pl);

/*notify_function*/
typedef int32_t (*SET_NOTIFY)(source_layer *pl, MSG_GATEWAY msg, void* key);

/*send comomand*/
typedef int32_t (*SEND_MSG)(source_layer *pl, int msgid, void *parm);

/*pause*/
typedef int32_t (*PAUSE)(source_layer *pl);

/*seek some frame*/
typedef int32_t (*SEEK)(source_layer *pl,int ms);

/*close a plugin*/
typedef int32_t (*CLOSE)(source_layer *pl);

/*customize a plugin*/
typedef int32_t (*CUSTOMIZE)(source_layer *pl, char*name, int *parm, int parm_cnt);

#define SOURCE_ID			FOURCC('S','O','U','R')	
#define RTMP_ID				FOURCC('R','T','M','P')
#define MP4_ID				FOURCC('M','P','4','_')
#define RTSP_ID				FOURCC('R','T','S','P')
#define AVFORMAT_ID			FOURCC('A','V','F','T')
#define HTTPSVR_ID			FOURCC('H','T','S','R')

struct source_layer
{
	/*normally*/
	OPEN		open;
	START		start;
	CLOSE		close;
	ISEOF		eof;
	SEEK		seek;
	PAUSE		pause;
	SET_NOTIFY	set_notify;
	SEND_MSG    send_msg;
	CUSTOMIZE	customize;
		
	void *plug_pri_data;
};

#endif //__SOURCE_LAYER_H__
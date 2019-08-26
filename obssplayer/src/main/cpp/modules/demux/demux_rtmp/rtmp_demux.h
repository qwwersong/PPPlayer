
#ifndef __RTMP_LIVE_DEMUX_H__
#define __RTMP_LIVE_DEMUX_H__

#include "../../../core/common.h"


#define RTMP_STREAM_NUM		2


#define BUFFER_SIZE			8192
#define	MAX_URL_SIZE		2000
#define RING_SIZE			25

#define SPACE_CHARS " \t\r\n"

#define URL_SCHEME_CHARS                        \
	"abcdefghijklmnopqrstuvwxyz"                \
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"                \
	"0123456789+-."

#define FFMIN(a,b) ((a) > (b) ? (b) : (a))

#if defined(_MSC_VER)
#define strtoll			_strtoi64
#define strcasecmp		_stricmp
#define strncasecmp		_strnicmp
#endif

typedef struct rtmp_stream
{
	//audio,video
	int			style;

	//compute time
	TIME_TYPE	stream_time;
	uint32_t	prev_time_stamp;
	int			diff;

	/* if it is present */
	int			present;

	uint8_t		extradata[64];
	int			extrasize;

	//used in wrap round
// 	int			wrap_round;
// 	uint32_t	stream_time2;
// 	uint32_t	prev_time_stamp2;
}rtmp_stream;

typedef struct rtmp_client
{
	/* rtmp handle */
	RTMP* rtmpHandle;

	/* thread is running */
	int running;

	/*rtmp URI */
	char uri[MAX_URL_SIZE];

	/* pause the stream */
	int pause;

	/* end of stream*/
	int eof;

	/* stream info */
	rtmp_stream stream[RTMP_STREAM_NUM];

	/*worker thread */
	THREAD_HANDLE_TYPE	worker_handle;
	THREAD_ID_TYPE pb_thread_id;

	/*notify callback*/
	MSG_GATEWAY notify_routine;
	void *key;

	/*data push callback*/
	FILLSAMPLE fillsample;
	void *fillkey;

	//0 flv head
	//1 ES tag
	int parse_step;

	// AVC sps ,pps
	uint8_t sequence_buf[512];
	int  sequence_size;

	/* push sps pps only once to player*/
	int	 push_avc_parms;

	/* the flag of connect rtmp server*/
	int	connect_server;
	
	/* tag of FLV file, include auido tag or video tag */
	flv_tag_struc flv_tag;

}rtmp_client;

typedef struct memory_cache{
	//init it
	uint8_t *buf_raw;
	int		buf_size;

	//
	uint8_t *start;
	uint8_t *end;
}memory_cache;

#ifdef __cplusplus
extern "C"
{
#endif

	void rtmp_stream_register(void *ptx);
	/* alloc cache_size bytes */
	void memory_init( memory_cache *h, int cache_size );
	void memory_reset( memory_cache *h );

	/* read some bytes from cache */
	int memory_read( memory_cache *h, uint8_t **buf, int size );

	/* read some bytes from cache */
	int memory_read_copy( memory_cache *h, uint8_t *buf, int size );

	/* move the memory to the start of cache*/
	uint8_t * memory_move( memory_cache *h, int *avail_size);

	/* read some new bytes */
	int memory_fill( memory_cache *h, int fill_size);

	/* release memory cache */
	void memory_uninit(memory_cache *h);

#ifdef __cplusplus
}
#endif


#endif //__RTMP_LIVE_DEMUX_H__
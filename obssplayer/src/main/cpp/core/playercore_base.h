#ifndef __VPC_BASE_H__
#define __VPC_BASE_H__

#include <android/data_type.h>
#include <sys/types.h>

#define VERSION_MAX_LENGTH 128
#define MAX_STREAM_NUM	2

/*msg between player core and demux plugins*/
#define MSG_AFTER_SEEK		0
#define MSG_MEDIA_FMT		1
#define MSG_BY_PASS			2
#define MSG_RECONNECT_SEEK	3
#define MSG_END_OF_FILE		4
#define MSG_GET_BUF_TIME	5 //get player core's buffer time in ms,   //also can get cur play time
#define MSG_GET_CORE_STATUS	6 //get player core's status
#define MSG_LIVE_RESET		7

/*current stream status*/
#define STREAM_STATUS_PLAY	1
#define STREAM_STATUS_PAUSE 2
#define STREAM_STATUS_STOP	3

#define MAX_PLAYER_STATUS_QUEUE	8

#define FLOW_COMPUTE_INTERVAL	1*1000 //(ms)
#define FLOW_SAMPLE_SIZES	3 
#define FPS_FRAMES			25 //frame compute range

#define FOURCCBE(a,b,c,d) \
	(((uint8_t)(a) << 24) | ((uint8_t)(b) << 16) | \
	((uint8_t)(c) << 8) | ((uint8_t)(d) << 0))

#define FOURCCLE(a,b,c,d) \
	(((uint8_t)(a) << 0) | ((uint8_t)(b) << 8) | \
	((uint8_t)(c) << 16) | ((uint8_t)(d)<< 24))

#ifdef IS_BIG_ENDIAN
#define FOURCC(a,b,c,d) FOURCCBE(a,b,c,d)
#else
#define FOURCC(a,b,c,d) FOURCCLE(a,b,c,d)
#endif

#define PRIORITY_STANDARD	500


/*internal process status*/
#define	ALL_OK			1
#define	BUFFER_FULL		2
#define	NO_MORE_DATA	3
#define CODEC_ERROR		4

//internal state for VPC_PS_PLAY or VPC_PS_PAUSE
#define Internal_Ready			0x1E0
#define Internal_Connecting		0x1EA
#define Internal_Media_Got		0x1EB
#define Internal_Enter_Buffer	0x1EC
#define Internal_Playing		0x1ED
#define Internal_Seeking		0x1EE
#define Internal_Live_Buffer	0x1EF

//skip frame type
typedef enum SkipFrame{
	skip_nothing=0,
	skip_show_frame,
	skip_video_frame,
	skip_to_I_frame,
}SkipFrame;

//
//about component manager
//
typedef struct node_boot_func
{
	const char *name;
	const char *desc;
	int category_id;
	int sub_id;
	int priority;
	void (*getversion)(char buffer[VERSION_MAX_LENGTH]);
	int (*available)(const long parm);
	void* (*create)(const void *parm, int size);
	void (*release)(void*p);
} node_boot_func;

typedef struct tools_def
{
	const node_boot_func *p;
	void* module_handle;
	struct tools_def *next;
}tools_def;

typedef struct audio_ext
{
	int freq;
	int channels;
	int bits;
	int bytes_per_second;
	uint8_t extradata[32];

	uint8_t *decode_buf;
	int decode_buf_size;
	ao_pkt	out_pkt;
    int mute;
}audio_ext;

typedef struct video_ext
{
	//picture size
	int width;
	int height;
	
	//make the width and height 2^n;
	int widthAlign2;
	int heightAlign2;

	//video frame section
	uint8_t *bufferbase; 
	uint8_t *sharpTempBuf;
	Picture *frame;
	int     in;
	int     out;
	int		pre_frame;
	int		buf_frame_cnt;
	int     avail;
	void	*avail_section;
	uint32_t already_ready;
	int     only_I_frame;

	/*time if only have video*/
	TIME_TYPE tick;
	TIME_TYPE time_ref;
    void *timer_section;

	/*FPS*/
	TIME_TYPE	showframes[FPS_FRAMES];
	int			idx;
	int			avail_val;
	float		fps;

	int		delivered;
	int		skipped;

	//	multi-thread decode
	THREAD_HANDLE_TYPE	video_handle;
	int					video_decode_pending;
	int					video_running;
	THREAD_ID_TYPE		video_thread_id;
	void *				video_semaphore;
	int					(*decode_routine)(void *, int);
	int					routine_ret;
	void *				decode_section;
    int                 sync_with_audio;
}video_ext;

typedef struct  stream_pack
{
	int			style;//audio,video
	uint8_t *	buffer;
	int32_t		buf_size;
	TIME_TYPE	dts;
	TIME_TYPE	pts;
	int			frame_style; //only for video ,'I','P','B'
	int			skippable;
	TIME_TYPE	stream_time; // core fill it.
	video_tag	vtag;
}stream_pack;

#define HISTROY_SPEED_CNT 3
typedef struct BufferSpeedComputer
{
	/*recent buffer change trend */
	int mHistroySpeed[HISTROY_SPEED_CNT];
	int mIn;
	int mSum;
	int mPrevious;
}BufferSpeedComputer;

typedef struct user_timer
{
	int			active;
	void	    *key;
	int			duration;
	TIME_TYPE	start_time;
	int			(*callback)(void *key );
}user_timer;

//
//about stream information
//
typedef struct media_stream
{
	/* stream style audio, video, text*/
	int				media_style;

	/* stream time */
	TIME_TYPE		recv_time;
	TIME_TYPE		use_time;

	/* the stream buffer time*/
	int				buffer_time;

	/* previous packet time */
	TIME_TYPE		prev_time_stamp;

	int				diff;

	int				stream_status;
	void*			mempool;
	void *			data_lock;

	/*codec layer*/
	codec_layer		*codec;
	const node_boot_func *codec_node;
	
	stream_pack		cur_pkt;
	int				output_offset;
	decode_context	dc;

	char			codecname[32];
	int				wait_for_output;
	SkipFrame		skip_frame;
	int				i_arrived; /*audio always 1,video according to frame style*/
	
	/*av render layer*/
	const node_boot_func*out_node;
	void	*out_device;
	
	/*stream speed*/
	int			requst_change_speed;
	fraction	request_speed;
	fraction	stream_speed;

	/*flow*/
	uint32_t	decoded_bytes;
	TIME_TYPE	flow_time;
	uint32_t	average_bitrate[FLOW_SAMPLE_SIZES];
	int			idx;
	int			cur_kbps;


	/*stream process*/
	int (*process)(struct media_stream *s, TIME_TYPE ps, int eof);

	/*process play speed*/
	void (*process_speed)(struct media_stream *s, int step);
	
	/*push all decoded data into A or V buffer*/
	int (*output)(struct media_stream *s,int finish);

	/*render audio or video*/
	int (*render)(struct media_stream *s, TIME_TYPE ps);
	
	/*close device*/
	void (*release)(struct media_stream *s);

	/*release avext*/
	void (*release_ext)(struct media_stream *s);

	void *avext;
}media_stream;

#define MAX_TIME_SIZE	12

typedef struct timestamp_reorder{
	TIME_TYPE time_array[MAX_TIME_SIZE];
	int		enter;
	int		leave;
}timestamp_reorder;

typedef struct req_msg{
    uint32_t cur; //如果不是0，表示需要在这个状态下，才能切换到req状态
	uint32_t req;
	int *wait;
}req_msg;

//
//main structure
//
typedef struct fmp_context
{
	/*manager of all nodes*/
	tools_def	*tools_start;
	
	/*object of player*/
	void		*player;

	/*source layer*/
	source_layer	*source;
	const node_boot_func *source_node;
	
	/*stream array, only support video and audio*/
	media_stream*	stream[MAX_STREAM_NUM];
	int				stream_num; //max is 2

	/*only a pointer,so upper must keep it OK*/
	player_parameter *urlinfo;

	TIME_TYPE		media_duration;

	/*player buffer time*/
	TIME_TYPE		should_buffer_time;
	TIME_TYPE		alreay_buffer_time;
	TIME_TYPE		got_data_time;

	/* about low latency play rtmp */
	user_timer		buffer_moniter_timer;
	int				low_latency_mode;

	 /* buffer exceed should buffer times */
	int				buffer_full;

	/*notify*/
	void  *			user_data;
	NOTIFY_CALLBACK notify_call_back;

	/*player current status*/
	int				vpc_cur_ps;
	void			*picture_lock;
	int				suspend_state;

	/*for Status machine*/
	req_msg			status_request_queue[MAX_PLAYER_STATUS_QUEUE];
	void			*status_lock;
    
    void            *req_queue_lock;
	int				request_index;
	int				processed_index;
	
	int				check_end_of_file;

	/* 
	 *	player decode mode status
	 *  0:audio video output buffer is ready
	 *  1:audio video output buffer is full, filled by decoded data
	 *  2:player have opened audio video device
	 */
	int				player_decode_mode_status;

	/* render thread flag */
	int				run_render_process;

	/* if allow render media */
	int				wait_surface_ready;

	/* decoder thread flag */
	int				run_decode_process;

	/* step for play */
	int				play_step;
    
    /*only loading movie open device and render first frame*/
    int				preload_mode;

	TIME_TYPE		position;
	TIME_TYPE		request_seek_pos;//request seek pos
	TIME_TYPE		seeked_pos;

	int				broken_continue;
	TIME_TYPE		do_seek_point;

	user_timer		check_net_timer;
	/* buffer speed computer */
	//BufferSpeedComputer	buffer_speed;

	//platform private data
	void*			platform_data;
 
}fmp_context;


/************************************************************************/
//					helper function declare
/************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
	void RegisterTools(const void*ptx, const void*node_func, void*PluginsHandle);

	const node_boot_func* find_codec(fmp_context *ptx, int32_t id);

	void	reset_request_status(fmp_context *ptx);
	req_msg	get_request_status(fmp_context*ptx);
	int		set_request_status(fmp_context*ptx, int cur,int req);
	int		set_request_status_wait(fmp_context*ptx, int cur,int req, int *wait);

	int32_t init_media_fmt(fmp_context *ptx, uint8_t *info, uint32_t len );
	int		create_stream(fmp_context *ptx,media_stream **st, AV_FMT *fmt,int blocksize,int ext_size);
	void	delete_stream(media_stream *s);

	int		low_level_buffer_ready( fmp_context *ptx );

	void compute_buffer_time(fmp_context *ptx);
	void process_error(fmp_context *ptx, int err);

	void vpc_printf(char *fmt,...);

#ifdef __cplusplus
}
#endif
#endif //__VPC_BASE_H__

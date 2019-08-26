
#ifndef __VIDEO_OUT_H__
#define __VIDEO_OUT_H__

/* Set up for C function definitions, even when using C++ */
#include <android/data_type.h>

#ifdef __cplusplus
extern "C" {
#endif

	int init_video(fmp_context *ptx,media_stream *s, void *parm, int parm_size);
	void uninit_video(media_stream *s);

	/*video driver interface*/
	int  open_video_device(media_stream *stream,void *extern_parm, int parm_size, int moniter);
	void close_video_device(media_stream *stream);

	/*video clock*/
	TIME_TYPE video_get_timer_value(media_stream *stream);
	void video_reset_timer(media_stream *stream,int b_clear_up);
	void video_reset_tick(media_stream *stream,TIME_TYPE tick);

	//frame buffer API
	int  video_push_image(media_stream *stream, decode_context*pdc,TIME_TYPE ts,video_tag *vt);
	int	 video_buffer_avail(media_stream *stream);
	int	 video_render_img(media_stream *stream,TIME_TYPE ps, int bShow);
	void video_flush_img(media_stream *stream);

	int makeAlgin2(int value);

	/*about timestamp order*/
	DYNAMIC_EXPORT void video_insert_ts(timestamp_reorder* t, TIME_TYPE time);
	DYNAMIC_EXPORT TIME_TYPE video_get_ts(timestamp_reorder *t );
	DYNAMIC_EXPORT void video_flush_ts(timestamp_reorder* t);

#ifdef __cplusplus
}
#endif

#endif //__VIDEO_OUT_H__
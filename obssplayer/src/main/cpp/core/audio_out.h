#ifndef __AUDIO_OUT_H__
#define __AUDIO_OUT_H__

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
	/* Function prototypes */
	int init_audio(fmp_context *ptx, media_stream *s,void *parm, int parm_size);
	void uninit_audio(media_stream *s);

	int open_audio_device(media_stream *stream,void *extern_parm, int parm_size, int moniter);
	void close_audio_device(media_stream *stream);

	int get_audio_buffer(audio_device *dev, ao_pkt *pkt);
	int push_audio_pkt(audio_device *dev, ao_pkt *pkt);
	void play_audio_pkt(audio_device *dev);

	/* get pending buffer size*/
	int  get_audio_pending_buffer_size(audio_device *dev);

	void pause_audio( audio_device *dev);
	void reset_audio( audio_device *dev );

    void set_audio_mute(media_stream *s, int mute);
	int set_audio_volume( audio_device *dev, int val );
	int get_audio_volume( audio_device *dev );

	void audio_flush_data(media_stream *s);

TIME_TYPE audio_get_timer_time(media_stream *stream);
	void audio_reset_timer(media_stream *stream,int b_clear_base);

	/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_OUT_H__ */

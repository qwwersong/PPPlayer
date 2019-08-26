#ifndef __AVOUT_H__
#define __AVOUT_H__

#define AV_DEVICE			FOURCC('A','V','D','_')
#define AUDIO_RENDER		FOURCC('A','R','E','D')
#define VIDEO_RENDER		FOURCC('V','R','E','D')

/*audio*/
typedef struct audio_out_spec {
	int32_t	freq;		/* DSP frequency -- samples per second */
	int32_t	format;		/* Audio data format,if every sample is 16bits, the value is 16. */
	int32_t channels;	/* Number of channels: 1 mono, 2 stereo */
	int32_t samples;	/* Audio buffer size in samples (power of 2) */
	uint32_t size;		/* Audio buffer size in bytes (calculated) */
} audio_out_spec;

typedef struct fraction
{
	uint32_t num;
	uint32_t den;
}fraction;

typedef struct AV_FMT
{
	int32_t			nType;			/*always be CONTENT_AUDIO (0x01) or CONTENT_VIDEO (0x02)  */
	int32_t			nFourCC;		/*CODEC identify, such as ' AAC'*/
	char			codecname[32];
	char			extradata[64];
	int				extrasize;
	union 
	{
		struct  // for audio  
		{
			int32_t			nChannels;
			int32_t			nSamplerate;
			int32_t			nBitsPerSample;
		}a;

		struct  // for video
		{        
			int32_t			nWidth;/*width in pixels*/
			int32_t			nHeight;/*height in pixels*/
			int				reserve;
		}v;
	}av;
}AV_FMT;

/*media format*/
typedef struct MEDIA_FMT
{
	uint32_t file_length; /*unit is MS*/
	AV_FMT fmt[3];
	int cnt;
}MEDIA_FMT;

typedef struct ao_pkt
{
	/*set by audio play module*/
	uint8_t*	ao_buf; //require from audio plugins
	int32_t		ao_size; //read only
	int32_t		tag;//read only
	
	//set by player core
	int32_t		ao_fill;
	TIME_TYPE	ref_time;
	fraction	speed;
}ao_pkt;

/*audio driver */
typedef struct audio_device audio_device;

#define AUDIO_OUT_PKT		0
#define AUDIO_OUT_FINISH	1

struct audio_device {
	/* * * */
	/* Public driver functions */
	int  (*open_audio)(audio_device *dev, audio_out_spec *spec);
	void (*close_audio)(audio_device *dev);	
	
	int (*get_audio_buffer)(audio_device *dev, ao_pkt *pkt);
	int (*push_audio_pkt)(audio_device *dev, ao_pkt *pkt);
	void (*play_audio_pkt)(audio_device *dev);


	void (*pause_audio)( audio_device *dev);
	void (*reset_audio)( audio_device *dev );


	/************************************************************************/
	/*   							audio play variable						*/
	/************************************************************************/

	void (*audio_msg)(void *key, int msgid, TIME_TYPE audio_time,fraction *speed);
	void *audio_key;

	audio_out_spec spec;

	/*current dev play speed*/
	fraction dev_speed;
	int	audio_moniter_mode;
    int audio_codec_id;

	/*audio clock*/
	void *		timer_section;
	TIME_TYPE	tick;
	TIME_TYPE	time_ref;

	/* waiting for play buffer */
	int			pending_buffer;

	void *extern_parm;
	int	exter_parm_size;

	/*private data*/
	void *		device_pdd;
};

/*video driver */

#define INVALID_TS			(~(TIME_TYPE)0x0)
#define EXCEED_RANGE		3000
#define EARLY_TOO_MUCH		40
#define DELAY_TOO_MUCH      100


/*only for debug delay*/
typedef struct video_tag
{
	TIME_TYPE rt; //receive time
	TIME_TYPE dt; //decode time
	TIME_TYPE st; //show time
	TIME_TYPE pt;//push time
}video_tag;

typedef struct Picture
{
	uint8_t *	src[3]; //y,u,v
	int			stride;
	TIME_TYPE	ts;
	video_tag	vtag;
}Picture;

typedef struct video_device video_device;


struct video_device {
	/*video interface*/
	int  (*open_video)(video_device *dev, int width, int height);
	void (*close_video)(video_device *dev);
	void (*render_img)(video_device *dev, uint8_t *src[3], int stride[3], int w, int h);

	/* * * */
	/* data common to all devices */
	void *extern_parm;
	int	exter_parm_size;

	int video_moniter_mode;
    int video_codec_id;

	/*private data*/
	void *device_pdd;
};


#endif//__AVOUT_H__

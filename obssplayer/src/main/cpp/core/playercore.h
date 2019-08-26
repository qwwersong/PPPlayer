#ifndef __VPC_H__
#define __VPC_H__

#ifdef __VPC_DLL__
	#define	VPC_FUNC DYNAMIC_EXPORT
#else
	#define VPC_FUNC
#endif //__VPC_DLL__

#define		VPC_OK							0
#define		VPC_OUT_OF_MEMORY				1
#define		VPC_NO_SOURCE_DEMUX				2
#define		VPC_NO_AUDIO_CODEC				3
#define		VPC_NO_VIDEO_CODEC				4

#define		VPC_CONNECT_SERVER				21
#define		VPC_NETWORK_ERROR				22
#define		VPC_MEDIA_SPEC_ERROR			23
#define		VPC_NO_PLAY_OBJECT				24
#define		VPC_NET_TIME_OUT				25
#define		VPC_RECV_PENDING				26
#define		VPC_DELETE_FRAME				27

#define		VPC_OPEN_AV_DEVICE				50
#define		VPC_NOTIFY_MEDIA_INFO			51
#define		VPC_START_BUFFER_DATA			52
#define		VPC_PRE_PLAY					53
#define		VPC_START_PLAY					54
#define		VPC_PLAY_FINISH					55
#define		VPC_LIVE_BUFFER					56

/*player state*/
#define		VPC_PS_UNKNOWN				0x00
#define 	VPC_PS_STOP					0x01
#define 	VPC_PS_PLAY					0x02
#define 	VPC_PS_PAUSE				0x03

#define		VPC_PS_COUNT			4

#define TM_MEDIA_VIDEO	0
#define TM_MEDIA_AUDIO	1
#define TM_MEDIA_TITLE	2


#define MED_FMT_AUTO	0
#define MED_FMT_HSM		1
#define MED_FMT_HLS		2


#ifdef __cplusplus
extern "C" {
#endif

	/*notify callback from player*/
	typedef void (*NOTIFY_CALLBACK)( void *user,int notify_id, unsigned int param );

	typedef struct player_parameter
	{
		char		play_url[2048];/*play URL*/
		int			format_selector; /*MED_FMT_AUTO,MED_FMT_HLS,MED_FMT_HSM*/
		int			start_pos;/*start point*/
		int			should_buffer_time; /*unit is millisecond*/
		void		*platform;

		int			moniter_mode;
	}player_parameter;

	typedef struct media_info
	{
		/*media length only for VOD*/
		unsigned int   dwDuration;/*unit is millisecond*/

		/*video*/
		int   video_available; /*1 or 0*/
		int   videoWidth;
		int   videoHeight;
		char  sz_video_name[32];

		/*audio*/
		int	audio_available; /*1 or 0 */
		int	freq;		/* DSP frequency -- samples per second */
		int	format;		/* Audio data format,if every sample is 16bits, the value is 16. */
		int channels;	/* Number of channels: 1 mono, 2 stereo */
		char sz_audio_name[32];
	}media_info;


	typedef struct player_status
	{
		unsigned int already_buffer_time;/*unit is millisecond*/
		unsigned int already_data_time;/*unit is millisecond*/
		unsigned int cur_play_pos;/*unit is millisecond*/

		/*video*/
		unsigned int delivered;
		unsigned int skipped;
		float	fps;
		int		v_kbps;
		int		a_kbps;
	}player_status;

	/************************************************************************/
	/*                      main API                                        */
	/************************************************************************/

	VPC_FUNC void* vpc_init(NOTIFY_CALLBACK func, void *user_data, void *platform_data);
	VPC_FUNC void vpc_start(void *pc,player_parameter*pp);

	VPC_FUNC void vpc_surface_ready(void *pc, int ready);

    //only load movie render first frame
    //resume play use vpc_start(pc,0)
    VPC_FUNC int vpc_load(void *pc,player_parameter*pp);
    
	VPC_FUNC int vpc_seek( void *pc,int ms );
	VPC_FUNC void vpc_pause(void *pc, int wait);


	/*step == 0, stop all*/
	/*step == 1, stop video render thread*/
	/*step == 2, stop rest*/
	VPC_FUNC void vpc_stop(void *pc);

	VPC_FUNC void vpc_get_media_info(void *pc, media_info* media);
	VPC_FUNC void vpc_get_status(void *pc,player_status*ps);

	VPC_FUNC int  vpc_get_player_state(void *pc);

    /*1 mute*/
    VPC_FUNC void vpc_audio_mute(void *pc, int mute);

	/* 1,only render I frame */
	VPC_FUNC void vpc_only_render_I_frame(void *pc, int flag);

	/*style is video or audio*/
	VPC_FUNC void vpc_open_av_device(void *pc, int style,void *parm, int parm_size);

	/* get version*/
	VPC_FUNC char* vpc_get_version(void *pc);

	VPC_FUNC void vpc_quit(void*pc);


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*__VPC_H__*/

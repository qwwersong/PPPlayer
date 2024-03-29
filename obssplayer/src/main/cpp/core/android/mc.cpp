﻿/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
#include "logger.h"
#include <pthread.h>
#include <stdio.h>
#include <android/native_window.h> // requires ndk r5 or newer
#include <android/native_window_jni.h> // requires ndk r5 or newer
#include <EGL/egl.h> // requires ndk r5 or newer
#include <GLES/gl.h>
#include <unistd.h>
#include <malloc.h>
#include <cstring>
#include "../android/playercore_platform.h"
#include "../../core/playercore.h"
#include "mc.h"

#define LOG_TAG "RTMP3"

/* in rtmp.so*/
extern "C"
{
	void __ares_library_init_jvm(JavaVM *jvm);
	int __ares_library_init_android(jobject connectivity_manager);
	int __ares_library_android_initialized(void);
	void __ares_library_cleanup_android(void);
}

extern JavaVM *			android_jvm;

typedef struct media_engine
{
	/*handle*/
	void * playerHandle;
	
	/* video audio */
	vpc_android_video	android_video;
	vpc_android_audio 	android_audio;

	player_parameter 	playerParm;

	/*msg dispatch*/
	pthread_t 			_threadId;
	int					msgLoopExit;
	int 				msgid;

	pthread_cond_t 		_cond;
	pthread_mutex_t  	_mutex;

	jobject				_msgJavaObj;
	int					have_player;

	int 				scale_type;
	int					surface_ready;

	int					player_run;
}media_engine;

#define EXIT_CODE	100000

static void* threadStartCallback(void *myself)
{
	media_engine * engine = (media_engine*)myself;
	JNIEnv *jni_env = 0;
	ATTACH_JVM(jni_env);
    jclass cls;
    jmethodID mid;
	
	//找到对应的类
    cls = jni_env->GetObjectClass(engine->_msgJavaObj);
    if(cls == NULL)
    {
        LOGE("FindClass() Error.....");
        goto on_error;
    }
    //再获得类中的方法
    mid = jni_env->GetMethodID(cls, "JNI_Callback", "(I)V");
    if (mid == NULL)
    {
        LOGE("GetMethodID() Error.....");
        goto on_error;
    }
	
	while(!engine->msgLoopExit)
	{
	
		pthread_mutex_lock(&engine->_mutex);
		while (engine->msgid == -1){
			pthread_cond_wait(&engine->_cond, &engine->_mutex);
		}
		
		if( engine->msgLoopExit ) break;
		
		LOG_INFO("send msg to java,msgid=%d",engine->msgid);
		
		jni_env->CallVoidMethod(engine->_msgJavaObj, mid, engine->msgid );

		engine->msgid = -1;
		pthread_mutex_unlock(&engine->_mutex);		
	}
	//LOG_INFO("Msg Thread exit");
on_error:
    DETACH_JVM(jni_env);
	return 0;
}

void put_msg( media_engine * engine,int msg )
{
	pthread_mutex_lock(&engine->_mutex);
	engine->msgid = msg;
	pthread_cond_signal(&engine->_cond);
	pthread_mutex_unlock(&engine->_mutex);	
}

void start_msg_thread(media_engine * engine)
{
    pthread_mutex_init(&engine->_mutex, 0);
	pthread_cond_init(&engine->_cond,0);
	engine->msgLoopExit = 0;
	engine->msgid = -1;
	pthread_create(&engine->_threadId, 0, threadStartCallback, engine);
}

void stop_msg_thread(media_engine * engine)
{
	engine->msgLoopExit = 1;
	put_msg( engine,EXIT_CODE );
    pthread_join(engine->_threadId, 0);	
    pthread_mutex_destroy(&engine->_mutex);
	pthread_cond_destroy(&engine->_cond);
	engine->msgid = -1;
}

static void playerMsgCallback( void *user,int notify_id, unsigned int param )
{
	media_engine * engine = (media_engine*)user;
	if(notify_id == VPC_OPEN_AV_DEVICE )
	{
        LOG_ERROR("初始化 音频、视频");
		int style = param;
		if(style == TM_MEDIA_VIDEO)
		{
			engine->android_video.scale_method = engine->scale_type;
			engine->android_video.mirror = 0;
			engine->android_video.changeSetting = 0;
            LOG_ERROR("open 视频");
			vpc_open_av_device(engine->playerHandle, TM_MEDIA_VIDEO, &engine->android_video, sizeof(engine->android_video));
		}
		if(style == TM_MEDIA_AUDIO)
		{
			engine->android_audio.android_jvm = android_jvm;
            LOG_ERROR("open 音频");
			vpc_open_av_device(engine->playerHandle, TM_MEDIA_AUDIO,&engine->android_audio, sizeof(engine->android_audio));
		}
		engine->player_run = 1;
	}
	//LOG_INFO("---------------callback code=%d",notify_id);
	put_msg(engine,notify_id);
}


JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
	android_jvm = vm; 
	LOG_INFO("JNI_OnLoad");
	return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved)
{
	android_jvm = 0;  
	LOG_INFO("JNI_OnUnload");
}

static media_engine* get_media_engine(JNIEnv * env, jobject thiz)
{
	jclass clazz = (env)->GetObjectClass(thiz);
	jfieldID field = (env)->GetFieldID(clazz, "mInstance", "I");
	jint me = (env)->GetIntField(thiz, field);
	return (media_engine*)me;
}

JNIEXPORT int JNICALL Java_nativeInterface_playerView_nativeOnCreate(JNIEnv* jenv, jobject thiz,jobject connectivity_manager)
{
	media_engine * engine = get_media_engine(jenv,thiz);
	if( !engine)
	{
		__ares_library_init_jvm(android_jvm);
		int ret = __ares_library_init_android(connectivity_manager);
		__ares_library_android_initialized();
		
		engine = (media_engine *)malloc(sizeof(media_engine));
		jclass clazz = (jenv)->GetObjectClass(thiz);
		jfieldID field = (jenv)->GetFieldID(clazz, "mInstance", "I");
		(jenv)->SetIntField(thiz, field, (jint) engine);
		if (engine == 0){
			LOG_INFO("nativeOnCreate can't malloc engine!");
			return 0;
		}
		memset(engine,0,sizeof(media_engine));
		pthread_mutex_init(&engine->android_video.render_mutex, 0);
		LOG_INFO("nativeOnCreate Object");
	}
	else
	{
		LOG_INFO("nativeOnCreate call more than one time before nativeOnDelete!");
	}
	return 1;
}

JNIEXPORT void JNICALL Java_nativeInterface_playerView_nativeOnResume(JNIEnv* jenv, jobject obj)
{
	LOG_INFO("nativeOnResume");
}

JNIEXPORT void JNICALL Java_nativeInterface_playerView_nativeOnPause(JNIEnv* jenv, jobject obj)
{
    LOG_INFO("Renderer thread stopped");
}

JNIEXPORT void JNICALL Java_nativeInterface_playerView_nativeOnDelete(JNIEnv* jenv, jobject thiz)
{
	jclass clazz = (jenv)->GetObjectClass(thiz);
	jfieldID field = (jenv)->GetFieldID(clazz, "mInstance", "I");
	media_engine * engine = get_media_engine(jenv,thiz);
	pthread_mutex_destroy(&engine->android_video.render_mutex);
	if (engine)
	{
		free(engine);
		(jenv)->SetIntField(thiz, field, (jint) 0);
	}
	LOG_INFO("nativeOnDelete");
	__ares_library_cleanup_android();
}

JNIEXPORT void JNICALL Java_nativeInterface_playerView_nativeSetSurface(JNIEnv* jenv, jobject obj, jobject surface)
{
	media_engine * engine = get_media_engine(jenv,obj);
	if( engine )
	{
		if (surface != 0) {
			engine->android_video._window = ANativeWindow_fromSurface(jenv, surface);
			LOG_INFO("Got window %p", engine->android_video._window);
			vpc_surface_ready(engine->playerHandle,1);
			engine->surface_ready = 1;
			//
		} else {
			LOG_INFO("Releasing window");
			if(engine->android_video._window)
				ANativeWindow_release(engine->android_video._window);
			engine->android_video._window = 0;
			vpc_surface_ready(engine->playerHandle,0);
			engine->surface_ready = 0;
		}
	}
}

JNIEXPORT void JNICALL Java_nativeInterface_playerView_nativeSurfaceChanged(JNIEnv* jenv, jobject obj,jobject surface)
{
	media_engine * engine = get_media_engine(jenv,obj);
	if( engine )
	{
		LOG_INFO("++++++++++++++surface changed!+++++++++++++++++++++");
		pthread_mutex_lock(&engine->android_video.render_mutex);

		Java_nativeInterface_playerView_nativeSetSurface(jenv,obj,0);
		Java_nativeInterface_playerView_nativeSetSurface(jenv,obj,surface);
		if (engine->android_video._window) {
			engine->android_video.surfacechanged = 1;
		}
		vpc_surface_ready(engine->playerHandle,1);
		engine->surface_ready = 1;
		pthread_mutex_unlock(&engine->android_video.render_mutex);
	}	
}
//////////////////////////////////////////////player interface///////////////////////////////////////

int readyPlayer(JNIEnv* jenv, jobject obj,media_engine * engine,jstring url, jint start,jint buftime)
{
	if(!engine->playerHandle)
		engine->playerHandle = vpc_init(playerMsgCallback,engine,0);
		
	if(!engine->playerHandle) 
	{
		LOG_INFO("Player init, out of memory!");
		return 0;
	}
	vpc_surface_ready(engine->playerHandle,engine->surface_ready);

	if( engine->have_player ){
		Java_nativeInterface_playerView_nativePlayerStop(jenv,obj);
	}
	engine->_msgJavaObj = jenv->NewGlobalRef(obj);
	start_msg_thread(engine);

	const char* szStr = jenv->GetStringUTFChars(url, 0);
	LOG_INFO( "play URL:%s\n", szStr );	
	strcpy(engine->playerParm.play_url,szStr);
	jenv->ReleaseStringUTFChars(url,szStr);

	engine->playerParm.format_selector = MED_FMT_AUTO;
	engine->playerParm.should_buffer_time = buftime;
	engine->playerParm.start_pos = start;
			
	return 1;
}

JNIEXPORT void JNICALL Java_nativeInterface_playerView_nativePlayerLoad(JNIEnv* jenv, jobject obj,jstring url, jint start,jint buftime,jint video_scale)
{
	LOG_INFO("nativePlayerLoad");
	media_engine * engine = get_media_engine(jenv,obj);
	if( engine ) 
	{
		if( !readyPlayer(jenv,obj,engine,url,start,buftime)){
			return;
		}
		engine->scale_type = video_scale;
		vpc_load(engine->playerHandle,&engine->playerParm);
		engine->have_player = 1;
	}
	else
	{
		LOG_INFO("no media engine in Player Start!");
	}
}
	
JNIEXPORT void JNICALL Java_nativeInterface_playerView_nativePlayerStart(JNIEnv* jenv, jobject obj,jstring url, jint start,jint buftime,jint video_scale)
{
	media_engine * engine = get_media_engine(jenv,obj);
	if( engine ) 
	{
		int wait_time = 0;

		if( !readyPlayer(jenv,obj,engine,url,start,buftime)){
			return;
		}
		engine->scale_type = video_scale;
		engine->player_run = 0;
		vpc_start(engine->playerHandle,&engine->playerParm);

		engine->have_player = 1;

		while ( !engine->player_run ){
			usleep(30*1000);
			wait_time += 30;
			if( wait_time > 400 ){
				engine->player_run = 1;
			}
		}
	}
	else
	{
		LOG_INFO("no media engine in Player Start!");
	}
	LOG_INFO("nativePlayerStart");
}

JNIEXPORT void JNICALL Java_nativeInterface_playerView_nativePlayerPause(JNIEnv* jenv, jobject obj,jint wait)
{
	media_engine * engine = get_media_engine(jenv,obj);
	if(engine)
	{
		if( engine->playerHandle ){
			vpc_pause(engine->playerHandle,wait);
		}
	}
	else
	{
		LOG_INFO("no media engine in Player Pause!");
	}
}

JNIEXPORT void JNICALL Java_nativeInterface_playerView_nativePlayerStop(JNIEnv* jenv, jobject obj)
{
	media_engine * engine = get_media_engine(jenv,obj);
	if( engine )
	{
		if(engine->have_player)
		{
			if( engine->playerHandle ){
				vpc_stop(engine->playerHandle);
			}
			stop_msg_thread(engine);
			if(engine->_msgJavaObj){
				jenv->DeleteGlobalRef( engine->_msgJavaObj );
			}

			engine->_msgJavaObj = 0;

			ANativeWindow *tmp = engine->android_video._window;
			memset(&engine->android_video,0,sizeof(engine->android_video));
			memset(&engine->android_audio,0,sizeof(engine->android_audio));
			engine->android_video._window = tmp;
			engine->have_player = 0;
			LOG_INFO("nativePlayerStop");
		}
	}
	else
	{
		LOG_INFO("no media engine in Player Stop!");
	}
}

JNIEXPORT jint JNICALL Java_nativeInterface_playerView_nativePlayerGetBufTime(JNIEnv* jenv, jobject obj)
{
	media_engine * engine = get_media_engine(jenv,obj);
	if( engine )
	{
		player_status ps={0};
		if( engine->playerHandle ){
			vpc_get_status( engine->playerHandle,&ps );
		}
		return ps.already_buffer_time;
	}
	else
	{
		LOG_INFO("no media engine in Player GetBufTime!");
		return 0;
	}
}

JNIEXPORT jint JNICALL Java_nativeInterface_playerView_nativePlayerGetPlayPos(JNIEnv* jenv, jobject obj)
{
	media_engine * engine = get_media_engine(jenv,obj);
	if( engine )
	{
		player_status ps={0};
		if( engine->playerHandle ){
			vpc_get_status( engine->playerHandle,&ps );
		}
		return ps.cur_play_pos;
	}
	else
	{
		LOG_INFO("no media engine in Player GetPlayPos!");
		return 0;
	}
}

JNIEXPORT void JNICALL Java_nativeInterface_playerView_nativePlayerMute(JNIEnv* jenv, jobject obj,jint mute)
{
	media_engine * engine = get_media_engine(jenv,obj);
	if(engine)
	{
		if( engine->playerHandle ){
			vpc_audio_mute(engine->playerHandle,mute);
		}
	}
	else
	{
		LOG_INFO("no media engine in Player mute!");
	}
}

JNIEXPORT void JNICALL Java_nativeInterface_playerView_nativePlayerResume(JNIEnv* jenv, jobject obj)
{
	media_engine * engine = get_media_engine(jenv,obj);
	if( engine )
	{
		if( engine->playerHandle )
			vpc_start(engine->playerHandle,0);
	}
	else
	{
		LOG_INFO("no media engine in Player resume!");
	}
}

JNIEXPORT jint JNICALL Java_nativeInterface_playerView_nativeGetVideoWidth(JNIEnv* jenv, jobject obj)
{
	media_engine * engine = get_media_engine(jenv,obj);
	if( engine )
	{
		media_info media={0};
		if( engine->playerHandle ){
			vpc_get_media_info(engine->playerHandle, &media);
		}
		return media.videoWidth;
	}
	else
	{
		LOG_INFO("no media engine in Player video width!");
		return 0;
	}
}

JNIEXPORT jint JNICALL Java_nativeInterface_playerView_nativeGetVideoHeight(JNIEnv* jenv, jobject obj)
{
	media_engine * engine = get_media_engine(jenv,obj);
	if( engine )
	{
		media_info media={0};
		if( engine->playerHandle ){
			vpc_get_media_info(engine->playerHandle, &media);
		}
		return media.videoHeight;
	}
	else
	{
		LOG_INFO("no media engine in Player video width!");
		return 0;
	}
}

JNIEXPORT jint JNICALL Java_nativeInterface_playerView_nativePlayerGetLength(JNIEnv* jenv, jobject obj)
{
	media_engine * engine = get_media_engine(jenv,obj);
	if( engine )
	{
		media_info media={0};
		if( engine->playerHandle ){
			vpc_get_media_info(engine->playerHandle, &media);
		}
		return media.dwDuration;
	}
	else
	{
		LOG_INFO("no media engine in Player GetLength!");
		return 0;
	}
}

JNIEXPORT jint JNICALL Java_nativeInterface_playerView_nativePlayerSeek(JNIEnv* jenv, jobject obj,jint ms)
{
	int ret = 0;
	media_engine * engine = get_media_engine(jenv,obj);
	if( engine )
	{
		if( engine->playerHandle )
			ret = vpc_seek(engine->playerHandle,ms);
	}
	else
	{
		LOG_INFO("no media engine in PlayerSeek!");
	}
	return ret;
}

JNIEXPORT jint JNICALL Java_nativeInterface_playerView_nativePlayerVideoScale(JNIEnv* jenv, jobject obj,jint scale)
{
	media_engine * engine = get_media_engine(jenv,obj);
	if( engine )
	{
		if( engine->have_player )
		{
			engine->android_video.scale_method = scale;
			engine->android_video.changeSetting = 1;
		}
		return 1;
	}
	else
	{
		LOG_INFO("no media engine in VideoScale!");
	}
	return 0;
}

/* 0  normal
   1  lef <---> right mirror
   */
JNIEXPORT jint JNICALL Java_nativeInterface_playerView_nativePlayerVideoMirror(JNIEnv* jenv, jobject obj,jint mirror)
{
	media_engine * engine = get_media_engine(jenv,obj);
	if( engine )
	{
		if( engine->have_player )
		{
			engine->android_video.mirror = mirror;
			engine->android_video.changeSetting = 1;
		}
		return 1;
	}
	else
	{
		LOG_INFO("no media engine in nativePlayerVideoMirror!");
	}
	return 0;
}

#ifndef __VPC_PLATFORM_H__
#define __VPC_PLATFORM_H__
#include <jni.h>
#include <android/native_window.h> // requires ndk r5 or newer
#include <android/native_window_jni.h> // requires ndk r5 or newer
#include <dlfcn.h>
#include <android/log.h>
#include <string.h>
#include <unistd.h> 
#include <stdint.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/stat.h>
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <ctype.h> 
#include <errno.h> 
#include <malloc.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <sys/ioctl.h> 
#include <stdarg.h> 
#include <fcntl.h> 
#include <pthread.h>

#define  ANDROID_VERSION 


#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

#define ATTACH_JVM(jni_env)  \
	bool  isAttached = false;  \
	int env_status = android_jvm->GetEnv((void **)&jni_env, JNI_VERSION_1_6); \
	if(env_status<0) {	\
		env_status = android_jvm->AttachCurrentThread(&jni_env,NULL);	\
		if(env_status>=0){	\
			isAttached = true;	\
		}	\
	}	\

#define DETACH_JVM(jni_env)   if( isAttached ){ android_jvm->DetachCurrentThread(); }


#define DYNAMIC_IMPORT
#define UINT64_C

typedef void (*AudioCallbackFunc)(void *user, int played,int channels, int samplerate, void *data, int size );
	
typedef struct vpc_android_audio
{
	JavaVM *android_jvm;
	
	/* audio data callback */
	AudioCallbackFunc 	hookFunc;
	void *user;
	
}vpc_android_audio;

#define    VIDEO_SCALE_ORIGINAL 0
#define    VIDEO_SCALE_FIT 		1
#define    VIDEO_SCALE_FILL 	2

typedef struct vpc_android_video
{
	ANativeWindow *		_window;
	int					scale_method;
	int 				mirror; //0 no, 1 左右镜像
	int 				changeSetting;
	
	//UI surface changed.
	int					surfacechanged;
	pthread_mutex_t  	render_mutex;
}vpc_android_video;


#endif //__VPC_PLATFORM_H__

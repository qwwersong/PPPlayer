#include "playercore_platform.h"
#include "../common.h"
#include "../../../../../../../../../../../Tools/NDK/android-ndk-r16b-windows-x86_64/android-ndk-r16b/sysroot/usr/include/jni.h"
#include "data_type.h"
#include "logger.h"

#define MAX_AUDIO_BLOCK	3

#define LOG_TAG "droid_audio"


extern JavaVM *android_jvm;

typedef struct wavebuffer
{
	TIME_TYPE 	RefTime;
	uint8_t*    pBuf;
	uint32_t	nBufLen;
	uint32_t	nAllocLen;
	fraction	wavebuffer_speed;
	int		  	nBlockStatus;/*0--avail, 1--pcm ready, 2--request 3--playing*/
} wavebuffer;


typedef struct WaveOut_droid
{	
	wavebuffer		wavearray[MAX_AUDIO_BLOCK];
	
	int				in;
	int				out;
	int				avail;
	void*         	pDataLock;
	
	void*	 		pAudioTrack;
	int  			nBufSize;
	int           	nBufTime;
	int				bInitPlay;
	
	uint8_t*		pRawdata;
	int				bEndOfPcm;
	
	//audio format in android audio
	int				channelFormat;
	int 			sampleFormat;
	
	//play thread info
	int				g_ThreadHandle;
	THREAD_ID_TYPE 	pb_thread_id;
	int 			nThreadStatus;    /* 0--inactive , 1--active */
	int 			nThreadQuitFlag; /* 1--exist, 0--quit  */
	
}WaveOut_droid;

static int  wave_open_audio(audio_device *dev, audio_out_spec *spec);
static void wave_close_audio(audio_device *dev);	

static int wave_get_audio_buffer(audio_device *dev, ao_pkt *pkt);
static int wave_push_audio_pkt(audio_device *dev, ao_pkt *pkt);
static void wave_play_audio_pkt(audio_device *dev);

static void wave_pause_audio( audio_device *dev);
static void wave_reset_audio( audio_device *dev );


static int Audio_Available(const long parm)
{
	LOGD("Audio_Available 0 \n");
	int request = parm;
	if( request != AUDIO_RENDER ) return 0;
	return 1;
}

static void *Audio_CreateDevice(const void *parm, int size)
{
	LOGD("Audio_CreateDevice 0 \n");
	audio_device *dev = (audio_device *)vpc_mem_alloc(sizeof(audio_device));
	if( !dev ) return 0;
	memset(dev, 0, sizeof(audio_device));

	dev->open_audio = wave_open_audio;
	dev->close_audio = wave_close_audio;

	dev->get_audio_buffer = wave_get_audio_buffer;
	dev->push_audio_pkt = wave_push_audio_pkt;
	dev->play_audio_pkt = wave_play_audio_pkt;

	dev->pause_audio = wave_pause_audio;
	dev->reset_audio = wave_reset_audio;


	return dev;
}

static void Audio_Release(void *p)
{
	LOGD("Audio_Release 0 \n");
	vpc_mem_free(p);
}

static const node_boot_func droid_waveout_boot_func =
{
	"android waveout",
	"Android WaveOut",
	AV_DEVICE,
	AUDIO_RENDER,
	PRIORITY_STANDARD,
	0,
	Audio_Available,
	Audio_CreateDevice,
	Audio_Release
};

void Android_Audio_register(void*ptx)
{
	LOGD("Android_Audio_register\n");
	RegisterTools(ptx, &droid_waveout_boot_func,0);
}

THREAD_RETURN_TYPE VPC_API ThreadWritePack( THREAD_PARAM_TYPE lpParameter )
{
	audio_device *dev = (audio_device*)lpParameter;
	WaveOut_droid *p_droid = (WaveOut_droid*)dev->device_pdd;
	
	JNIEnv *jni_env = 0;
	ATTACH_JVM(jni_env);
	jmethodID write_method=0, play_method=0, init_method=0,stop_method=0;
    jclass jcl;
	int status;
	jbyte* buf;
	jbyteArray outputBuffer;
	jclass 	track_class;
	jobject audio_track;
	
   /* Find audio track class and create global ref */
	jcl = jni_env->FindClass("android/media/AudioTrack");
	if (jcl == NULL) {
		LOGD("Unable to find audio track class");
		goto on_error;
	}
	LOGD("get audio track,now begin to config it");
	track_class = (jclass)jni_env->NewGlobalRef(jcl);
	jni_env->DeleteLocalRef(jcl);
	if (track_class == 0) {
		goto on_error;
	}
	
	init_method = jni_env->GetMethodID(track_class, "<init>",
		"(IIIIII)V");
	
	LOGD("begin to create audio track");
	audio_track = jni_env->NewObject(track_class,
			init_method,
			3, 			  /*AudioManager.STREAM_MUSIC*/
			dev->spec.freq,   /*sampleRateInHz*/
			p_droid->channelFormat,/*CHANNEL_CONFIGURATION_MONO*/
			p_droid->sampleFormat, /*ENCODING_PCM_16BIT*/
			p_droid->nBufSize,  /*bufferSizeInBytes*/
			1			  /*AudioTrack.MODE_STREAM*/
			);
		
	if(!audio_track)
		LOGE("can't create audio track");
		
	//play
     play_method = jni_env->GetMethodID(track_class, "play","()V");

	 jni_env->CallVoidMethod(audio_track, play_method);
 

    //write
    write_method = jni_env->GetMethodID(track_class,"write","([BII)I");

	outputBuffer = jni_env->NewByteArray(p_droid->nBufSize);
	if (outputBuffer == 0) {
		LOGD("Not able to allocate a buffer for input play process");
		goto on_error;
	}
	buf = jni_env->GetByteArrayElements(outputBuffer, 0);

	while(p_droid->nThreadQuitFlag)
	{
		if(p_droid->nThreadStatus) // active
		{
			wavebuffer *p = &p_droid->wavearray[p_droid->out];
			if(p->nBlockStatus == 2)
			{
				p->nBlockStatus = 3;// playing
				
				memcpy(buf,p->pBuf,p_droid->nBufSize);
				status = jni_env->CallIntMethod(audio_track, write_method,
										outputBuffer,
										0,
										p_droid->nBufSize);
				//((AudioTrack*)p_droid->pAudioTrack)->write(p->pBuf, p_droid->nBufSize); // this func is block

				p->nBlockStatus = 0;//avail
				
				p_droid->out++;
				if(p_droid->out>=MAX_AUDIO_BLOCK) p_droid->out=0;
				
				if(p_droid->bInitPlay == 1)
				{
					p_droid->bInitPlay = 0;
				}else
				{
					p->RefTime = p->RefTime-p_droid->nBufTime;
					dev->audio_msg(dev->audio_key, AUDIO_OUT_PKT, p->RefTime, &p->wavebuffer_speed);
					p->RefTime = 0;
				}	
				
				mutex_lock(p_droid->pDataLock);
				//if(p_droid->avail<3)  // protect avail reset 3 plus 1 while SEEK between two threads switch
				p_droid->avail++;
				if( p_droid->bEndOfPcm && p_droid->avail == MAX_AUDIO_BLOCK )
				{
					dev->audio_msg(dev->audio_key,AUDIO_OUT_FINISH,0,0);
				}
				mutex_unlock(p_droid->pDataLock);

			}
			usleep(10);
		}
		else // inacitve
		{
			usleep(5*1000); 
		}
	}
	
	jmethodID method_id;
	method_id = jni_env->GetMethodID(track_class,"flush", "()V");

	jni_env->CallVoidMethod(audio_track, method_id);
	method_id = jni_env->GetMethodID(track_class, "stop", "()V");
	jni_env->CallVoidMethod(audio_track, method_id);
	
	//don't forget release. otherwise memory leak!!!
	method_id = jni_env->GetMethodID(track_class, "release", "()V");
	if(method_id)
		jni_env->CallVoidMethod(audio_track, method_id);


	jni_env->ReleaseByteArrayElements(outputBuffer, buf, 0);
	jni_env->DeleteLocalRef(outputBuffer);
	jni_env->DeleteLocalRef(audio_track);
	jni_env->DeleteGlobalRef(track_class);
	
on_error:
    DETACH_JVM(jni_env);
	LOGD("good bye audio play thread9");

	return 0;
}

int  wave_open_audio(audio_device *dev, audio_out_spec *spec)
{
	WaveOut_droid* p_droid = (WaveOut_droid*)vpc_mem_alloc(sizeof(WaveOut_droid));
	if( !p_droid)
		return 0;

	dev->device_pdd = p_droid;

	vpc_android_audio* pAndroid_audio_parm = (vpc_android_audio*) dev->extern_parm;
	JNIEnv *jni_env = 0;
	ATTACH_JVM(jni_env);
	
	jmethodID min_buff_size_id;
	
	dev->spec = *spec;
	   
	/* Find audio track class and create global ref */
	jclass jcl = jni_env->FindClass("android/media/AudioTrack");
	if (jcl == NULL) {
		LOGD("Unable to find audio track class");
		goto on_error;
	}
	
	if (spec->format == 8) {
		p_droid->sampleFormat = 3; //ENCODING_PCM_8BIT
	} 
	else if (spec->format == 16) {
		p_droid->sampleFormat = 2; //ENCODING_PCM_16BIT
	}
	
	if (spec->channels == 1) {
		p_droid->channelFormat = 2; //CHANNEL_CONFIGURATION_MONO
	}
	else if (spec->channels == 2) {
		p_droid->channelFormat = 3; //CHANNEL_CONFIGURATION_STEREO
	}
	
	min_buff_size_id  = jni_env->GetStaticMethodID(
										 jcl,
										"getMinBufferSize",
										"(III)I");
	//audio driver size
	p_droid->nBufSize = jni_env->CallStaticIntMethod(jcl,min_buff_size_id,
			    spec->freq,
			    p_droid->channelFormat,
				p_droid->sampleFormat);
			
	if( jcl ) {
		jni_env->DeleteLocalRef(jcl);
		jcl = 0;
	}
	//为了避免采样率太小，系统获取出来的缓冲区太小，
	//在某些设备上没声音
	if(spec->freq == 8000)
		p_droid->nBufSize *=5;
	else if( spec->freq == 16000 )
		p_droid->nBufSize *=1;
	else if( spec->freq == 24000 )
		p_droid->nBufSize *=2;
	else if( spec->freq == 36000 )
		p_droid->nBufSize *=2;
	
	if( spec->channels == 1 )
		p_droid->nBufSize *=2;
	
	p_droid->nBufSize *=2;
	
	LOGD("p_droid->nBufSize=%d",p_droid->nBufSize);
	//androi audio driver delay
	p_droid->nBufTime = p_droid->nBufSize*1000/(spec->freq*spec->channels*spec->format/8);

	p_droid->pRawdata = (uint8_t*)vpc_mem_alloc(p_droid->nBufSize*MAX_AUDIO_BLOCK);
	if(p_droid->pRawdata != NULL)
	{
		wavebuffer *wb = 0;
		for (int i=0;i<MAX_AUDIO_BLOCK;i++)
		{
			wb = &p_droid->wavearray[i];
			wb->RefTime = 0;
			wb->nAllocLen = p_droid->nBufSize;
			wb->pBuf = p_droid->pRawdata+i*p_droid->nBufSize; 
			wb->nBufLen = 0;
			wb->nBlockStatus = 0;
			wb->wavebuffer_speed.num = 1;
			wb->wavebuffer_speed.den = 1;
		}
	}

	p_droid->in	= 0; 
	p_droid->out = 0;
	p_droid->avail = MAX_AUDIO_BLOCK;
	p_droid->pDataLock = mutex_init();
		
	p_droid->bInitPlay = 1;
	p_droid->bEndOfPcm = 0;
	
	p_droid->nThreadStatus = 0; // inactive
	LOGD("wave_open_audio,avail=%d \n", p_droid->avail);
    DETACH_JVM(jni_env);
	return 1;
	
on_error:
    DETACH_JVM(jni_env);
	return 0;
}

void wave_close_audio(audio_device *dev)
{
	WaveOut_droid *p_droid = (WaveOut_droid*)dev->device_pdd;
	if(!p_droid) return;
	LOGD("wave_close_audio111");
	p_droid->nThreadQuitFlag = 0;
	vpc_thread_destory(&p_droid->g_ThreadHandle, p_droid->pb_thread_id);
	vpc_mem_free(p_droid->pRawdata);
	vpc_mem_free(p_droid);
	dev->device_pdd = NULL;	
	LOGD("wave_close_audio");
}

int wave_get_audio_buffer(audio_device *dev, ao_pkt *pkt)
{	
	int ret = 0;
	WaveOut_droid *p_droid = (WaveOut_droid*)dev->device_pdd;		
	if(p_droid->avail>0)
	{
		wavebuffer *p = &p_droid->wavearray[p_droid->in];
		pkt->ao_buf =  p->pBuf;
		pkt->ao_size = p->nAllocLen;
		pkt->ref_time = 0;
		pkt->tag = p_droid->in;
		ret=1;
	}
	return ret;
}

int wave_push_audio_pkt(audio_device *dev, ao_pkt *pkt)
{
	//LOGD("wave_push_audio_pkt 0 \n");
	WaveOut_droid *p_droid = (WaveOut_droid*)dev->device_pdd;
	wavebuffer *p	= &p_droid->wavearray[p_droid->in];

	if( pkt )
	{
		//the last pkt size(au_fill) may not equel the size of block.
		p->nBufLen = pkt->ao_fill; 
		p->RefTime = pkt->ref_time;
		p->wavebuffer_speed = pkt->speed;
		mutex_lock(p_droid->pDataLock);
		p_droid->in++;
		if(p_droid->in>=MAX_AUDIO_BLOCK) p_droid->in =0;
		p_droid->avail--;	
		mutex_unlock(p_droid->pDataLock);

		p->nBlockStatus = 1;/*pcm data ready*/
	}
	else
	{
		p_droid->bEndOfPcm = 1; 
		//prevent from missing AUDIO_OUT_FINISH
		//while decoder and render thread switching.
		if( p_droid->avail == MAX_AUDIO_BLOCK){
			dev->audio_msg(dev->audio_key,AUDIO_OUT_FINISH,0,0);
		}
	}
	return 1;
}

void wave_play_audio_pkt(audio_device *dev)
{
	//LOGD("wave_play_audio_pkt 0 \n");
	WaveOut_droid *p_droid = (WaveOut_droid*)dev->device_pdd;
	if( !p_droid) return;
	wavebuffer *p = &p_droid->wavearray[p_droid->out];
	if(p->nBlockStatus == 1) //pcm ready
	{
		p->nBlockStatus = 2; //request to play
	}

	if(p_droid->nThreadStatus == 0) 
	{
		p_droid->nThreadStatus = 1; 
		if(p_droid->nThreadQuitFlag == 0)
		{
			p_droid->nThreadQuitFlag = 1; // exist
			vpc_thread_create(&p_droid->pb_thread_id, ThreadWritePack, dev);
			
		}	
	}
}

void wave_pause_audio( audio_device *dev)
{
	//LOGD("wave_pause_audio 0 \n");
	WaveOut_droid *p_droid = (WaveOut_droid*)dev->device_pdd;
	if( !p_droid) return;
	if(p_droid->bEndOfPcm != 1){
		//LOGD("BUG--wave_pause_audio pause() \n");
		//((AudioTrack*)(p_droid->pAudioTrack))->pause();
	}
	p_droid->nThreadStatus = 0;
	LOGD("wave_pause_audio");
}

void wave_reset_audio( audio_device *dev )
{
	LOGD("wave_reset_audio 0 \n");
	WaveOut_droid *p_droid = (WaveOut_droid*)dev->device_pdd;	
	if( !p_droid) return;
	LOGD("wave_reset_audio thread_destory");
	p_droid->nThreadStatus = 0;
	p_droid->nThreadQuitFlag = 0;
	vpc_thread_destory(&p_droid->g_ThreadHandle, p_droid->pb_thread_id);
	
	mutex_lock(p_droid->pDataLock);
	p_droid->in	= 0;
	p_droid->out	= 0;
	p_droid->avail = MAX_AUDIO_BLOCK;

	wavebuffer *wb = 0;
	for (int i=0;i<MAX_AUDIO_BLOCK;i++)
	{
		wb = &p_droid->wavearray[i];
		wb->nBlockStatus = 0;
	}
	
	mutex_unlock(p_droid->pDataLock);
	
	p_droid->bInitPlay = 0;
	p_droid->bEndOfPcm = 0;
}

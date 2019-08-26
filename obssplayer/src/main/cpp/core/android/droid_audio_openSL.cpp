#include "common.h"
#include "playercore_platform.h"
#include <assert.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#define MAX_AUDIO_BLOCK	5
#define AUDIO_BLOCK_SAMPLES	 4096
#define LOG_TAG "droid_audio_opensl"

extern JavaVM *android_jvm;

typedef struct wavebuffer
{
	TIME_TYPE 	RefTime;
	uint8_t*    pBuf;
	uint32_t	nBufLen;
	uint32_t	nAllocLen;
	fraction	wavebuffer_speed;
} wavebuffer;


// aux effect on the output mix, used by the buffer queue player
static const SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

typedef struct WaveOut_droid
{	
	wavebuffer	wavearray[MAX_AUDIO_BLOCK];
	// engine interfaces
	SLObjectItf	engineObject;
	SLEngineItf	engineEngine;

	// output mix interfaces
	SLObjectItf	outputMixObject;
	SLEnvironmentalReverbItf outputMixEnvironmentalReverb;

	// buffer queue player interfaces
	SLObjectItf		bqPlayerObject;
	SLPlayItf		bqPlayerPlay;

	SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;

	SLEffectSendItf	bqPlayerEffectSend;
	SLMuteSoloItf	bqPlayerMuteSolo;

	SLVolumeItf		bqPlayerVolume;

	int				out_in;
	int				out_out;

	int				push_array[MAX_AUDIO_BLOCK];
	int				push_cnt;

	int				avail;
	void*         	pDataLock;

	int  			nBufSize;
	int           	nBufTime;
	
	uint8_t*		pRawdata;
	int				bEndOfPcm;
	
	//audio format in android audio
	int				channelFormat;
	int 			sampleFormat;
	
	// is it playing
	int				running;
}WaveOut_droid;

//--------------------------------------------------------------

static int  wave_open_audio(audio_device *dev, audio_out_spec *spec);
static void wave_close_audio(audio_device *dev);	

static int wave_get_audio_buffer(audio_device *dev, ao_pkt *pkt);
static int wave_push_audio_pkt(audio_device *dev, ao_pkt *pkt);
static void wave_play_audio_pkt(audio_device *dev);

static void wave_pause_audio( audio_device *dev);
static void wave_reset_audio( audio_device *dev );

//--------------------------------------------------------------

static int Audio_Available(const long parm)
{
	LOGE("Audio_Available 0 \n");
	int request = parm;
	if( request != AUDIO_RENDER ) return 0;
	return 1;
}

static void *Audio_CreateDevice(const void *parm, int size)
{
	LOGE("Audio_CreateDevice 0 \n");
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
	vpc_mem_free(p);
}

static const node_boot_func droid_waveout_boot_func =
{
	"android waveout OpenSL ES",
	"android waveout OpenSL ES",
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
	RegisterTools(ptx, &droid_waveout_boot_func,0);
}

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	audio_device *dev = (audio_device*)context;
	WaveOut_droid *p_droid = (WaveOut_droid*)dev->device_pdd;
	wavebuffer *p = &p_droid->wavearray[p_droid->out_out];
	vpc_android_audio *android_audio = (vpc_android_audio*)dev->extern_parm;
	
	//p->RefTime = p->RefTime-p_droid->nBufTime;
	dev->audio_msg(dev->audio_key, AUDIO_OUT_PKT, p->RefTime, &p->wavebuffer_speed);
	//LOGE("bqPlayerCallback %d block,time=%d",p_droid->out_out,p->RefTime);
	p->RefTime = 0;

	if( ++p_droid->out_out >= MAX_AUDIO_BLOCK ){
		p_droid->out_out = 0;
	}

	mutex_lock(p_droid->pDataLock);
	p_droid->avail++;
	//LOGE("in audio callback p_droid->avail =%d\n", p_droid->avail);
	if( p_droid->bEndOfPcm && p_droid->avail == MAX_AUDIO_BLOCK ){
		dev->audio_msg( dev->audio_key,AUDIO_OUT_FINISH, 0, 0 );
	}
	mutex_unlock(p_droid->pDataLock);
}

int  wave_open_audio(audio_device *dev, audio_out_spec *spec)
{
	WaveOut_droid* p_droid = (WaveOut_droid*)vpc_mem_alloc(sizeof(WaveOut_droid));;
	if( !p_droid)
		return 0;
	memset(&p_droid->wavearray, 0, sizeof(p_droid->wavearray));
	//vpc_android_audio* pAndroid_audio_parm = (vpc_android_audio*) dev->extern_parm;
	dev->device_pdd = p_droid;
	p_droid->running = 0;
	dev->spec = *spec;

    SLresult result;

    // create engine
    result = slCreateEngine(&p_droid->engineObject, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the engine
    result = (*p_droid->engineObject)->Realize(p_droid->engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the engine interface, which is needed in order to create other objects
    result = (*p_droid->engineObject)->GetInterface(p_droid->engineObject, SL_IID_ENGINE, &p_droid->engineEngine);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // create output mix, with environmental reverb specified as a non-required interface
    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    result = (*p_droid->engineEngine)->CreateOutputMix(p_droid->engineEngine, &p_droid->outputMixObject, 1, ids, req);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the output mix
    result = (*p_droid->outputMixObject)->Realize(p_droid->outputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the environmental reverb interface
    // this could fail if the environmental reverb effect is not available,
    // either because the feature is not present, excessive CPU load, or
    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
    result = (*p_droid->outputMixObject)->GetInterface(p_droid->outputMixObject, SL_IID_ENVIRONMENTALREVERB,&p_droid->outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
        result = (*p_droid->outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                p_droid->outputMixEnvironmentalReverb, &reverbSettings);
        (void)result;
    }
	
	
	// configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, MAX_AUDIO_BLOCK};
    SLDataFormat_PCM format_pcm = {	SL_DATAFORMAT_PCM, 
									1,
									SL_SAMPLINGRATE_8,
									SL_PCMSAMPLEFORMAT_FIXED_16,
									SL_PCMSAMPLEFORMAT_FIXED_16,
									SL_SPEAKER_FRONT_CENTER,
									SL_BYTEORDER_LITTLEENDIAN};
									
	
	switch (spec->freq)
	{
		case 8000:
			format_pcm.samplesPerSec = SL_SAMPLINGRATE_8;
			break;
		case 16000:
			format_pcm.samplesPerSec = SL_SAMPLINGRATE_16;
			LOGE("PCM sample rate--->SL_SAMPLINGRATE_16");
			break;
		case 22050:
			format_pcm.samplesPerSec = SL_SAMPLINGRATE_22_05;
			break;
		case 24000:
			format_pcm.samplesPerSec = SL_SAMPLINGRATE_24;
			break;
		case 32000:
			format_pcm.samplesPerSec = SL_SAMPLINGRATE_32;
			break;
		case 44100:
			format_pcm.samplesPerSec = SL_SAMPLINGRATE_44_1;
			break;
		case 48000:
			format_pcm.samplesPerSec = SL_SAMPLINGRATE_48;
			break;			
		default:
			LOGE("unsupported sample rate %d",spec->freq);
			return 0;
//			break;
	}
	
	if( spec->channels == 1 || spec->channels == 2 ){
		if( spec->channels == 2 ){
			format_pcm.channelMask =  SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
		}
		else{
			format_pcm.channelMask =  SL_SPEAKER_FRONT_CENTER;
		}
		format_pcm.numChannels = spec->channels;
	}
	else {
		LOGE("unsupported channels %d",spec->channels);
		return 0;
	}
	
	p_droid->nBufSize = AUDIO_BLOCK_SAMPLES*spec->channels*2;
		
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, p_droid->outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    // create audio player
    const SLInterfaceID ids_[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
            /*SL_IID_MUTESOLO,*/ SL_IID_VOLUME};
    const SLboolean req_[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
            /*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE};
    result = (*p_droid->engineEngine)->CreateAudioPlayer(p_droid->engineEngine, &p_droid->bqPlayerObject, &audioSrc, &audioSnk,
            3, ids_, req_);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
    // realize the player
    result = (*p_droid->bqPlayerObject)->Realize(p_droid->bqPlayerObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
    // get the play interface
    result = (*p_droid->bqPlayerObject)->GetInterface(p_droid->bqPlayerObject, SL_IID_PLAY, &p_droid->bqPlayerPlay);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the buffer queue interface
    result = (*p_droid->bqPlayerObject)->GetInterface(p_droid->bqPlayerObject, SL_IID_BUFFERQUEUE, &p_droid->bqPlayerBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // register callback on the buffer queue
    result = (*p_droid->bqPlayerBufferQueue)->RegisterCallback(p_droid->bqPlayerBufferQueue, bqPlayerCallback, dev);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the effect send interface
    result = (*p_droid->bqPlayerObject)->GetInterface(p_droid->bqPlayerObject, SL_IID_EFFECTSEND,
            &p_droid->bqPlayerEffectSend);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

#if 0   // mute/solo is not supported for sources that are known to be mono, as this is
    // get the mute/solo interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_MUTESOLO, &bqPlayerMuteSolo);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
#endif

    // get the volume interface
    result = (*p_droid->bqPlayerObject)->GetInterface(p_droid->bqPlayerObject, SL_IID_VOLUME, &p_droid->bqPlayerVolume);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
		
	//android audio driver delay
	p_droid->nBufTime = p_droid->nBufSize*1000/(spec->freq*spec->channels*spec->format/8);
	vpc_printf("p_droid->nBufSize=%d,every block time%d",p_droid->nBufSize, p_droid->nBufTime);

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
			wb->wavebuffer_speed.num = 1;
			wb->wavebuffer_speed.den = 1;
		}
	}

	p_droid->out_in = 0;
	p_droid->out_out = 0;
	p_droid->avail = MAX_AUDIO_BLOCK;
	p_droid->pDataLock = mutex_init();
		
	p_droid->bEndOfPcm = 0;
	LOGE("wave_open_audio,avail=%d \n", p_droid->avail);
	return 1;
	

	return 0;
}

void wave_close_audio(audio_device *dev)
{
	WaveOut_droid *p_droid = (WaveOut_droid*)dev->device_pdd;
	if(!p_droid) return;
	
    // destroy buffer queue audio player object, and invalidate all associated interfaces
    if (p_droid->bqPlayerObject != NULL) {
        (*p_droid->bqPlayerObject)->Destroy(p_droid->bqPlayerObject);
        p_droid->bqPlayerObject = NULL;
        p_droid->bqPlayerPlay = NULL;
        p_droid->bqPlayerBufferQueue = NULL;
    }

    // destroy output mix object, and invalidate all associated interfaces
    if (p_droid->outputMixObject != NULL) {
        (*p_droid->outputMixObject)->Destroy(p_droid->outputMixObject);
        p_droid->outputMixObject = NULL;
    }

    // destroy engine object, and invalidate all associated interfaces
    if (p_droid->engineObject != NULL) {
        (*p_droid->engineObject)->Destroy(p_droid->engineObject);
        p_droid->engineObject = NULL;
        p_droid->engineEngine = NULL;
    }
	vpc_mem_free(p_droid->pRawdata);
	vpc_mem_free(p_droid);
	dev->device_pdd = NULL;	
}

int wave_get_audio_buffer(audio_device *dev, ao_pkt *pkt)
{	
	int ret = 0;
	WaveOut_droid *p_droid = (WaveOut_droid*)dev->device_pdd;		
	if(p_droid->avail>0)
	{
		wavebuffer *p = &p_droid->wavearray[p_droid->out_in];
		pkt->ao_buf		=  p->pBuf;
		pkt->ao_size	= p->nAllocLen;
		pkt->ref_time	= 0;
		pkt->tag		= p_droid->out_in;

		mutex_lock(p_droid->pDataLock);
		if( ++p_droid->out_in >= MAX_AUDIO_BLOCK ){
			p_droid->out_in = 0;
		}
		p_droid->avail--;
		mutex_unlock(p_droid->pDataLock);

		ret=1;
	}
	return ret;
}

int wave_push_audio_pkt(audio_device *dev, ao_pkt *pkt)
{
	WaveOut_droid *p_droid = (WaveOut_droid*)dev->device_pdd;

	if( pkt )
	{
		wavebuffer *p	= &p_droid->wavearray[pkt->tag];
		//the last pkt size(au_fill) may not equel the size of block.
		p->nBufLen = pkt->ao_fill; 
		p->RefTime = pkt->ref_time;
		p->wavebuffer_speed = pkt->speed;
		mutex_lock(p_droid->pDataLock);
		p_droid->push_array[p_droid->push_cnt++] = pkt->tag;
		mutex_unlock(p_droid->pDataLock);
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
	WaveOut_droid *p_droid = (WaveOut_droid*)dev->device_pdd;
	if( !p_droid) 
		return;
	wavebuffer *p;

	//vpc_android_audio *android_audio = (vpc_android_audio*)dev->extern_parm;

	if( p_droid->running == 0 ){
		SLresult result = (*p_droid->bqPlayerPlay)->SetPlayState(p_droid->bqPlayerPlay, SL_PLAYSTATE_PLAYING);
		assert(SL_RESULT_SUCCESS == result);
		p_droid->running = 1;
		vpc_printf("let opensL running");
	}
	mutex_lock(p_droid->pDataLock);
	for (int i=0; i < p_droid->push_cnt; i++)
	{
		p = &p_droid->wavearray[p_droid->push_array[i]];
		SLresult result = (*p_droid->bqPlayerBufferQueue)->Enqueue(p_droid->bqPlayerBufferQueue, p->pBuf, p_droid->nBufSize );
	}
	p_droid->push_cnt = 0;
	mutex_unlock(p_droid->pDataLock);
}

void wave_pause_audio( audio_device *dev )
{
	//LOGE("wave_pause_audio 0 \n");
	WaveOut_droid *p_droid = (WaveOut_droid*)dev->device_pdd;
	if( !p_droid) return;
	if(p_droid->bEndOfPcm != 1){
		//LOGE("BUG--wave_pause_audio pause() \n");
		//((AudioTrack*)(p_droid->pAudioTrack))->pause();
	}
		
	SLresult result;
	result = (*p_droid->bqPlayerPlay)->SetPlayState(p_droid->bqPlayerPlay, SL_PLAYSTATE_PAUSED);
	assert(SL_RESULT_SUCCESS == result);
	p_droid->running = 0;
	LOGE("wave_pause_audio");
}

void wave_reset_audio( audio_device *dev )
{
	LOGE("+++++++++wave_reset_audio 0 \n");
	WaveOut_droid *p_droid = (WaveOut_droid*)dev->device_pdd;	
	if( !p_droid) return;

	SLresult result;
	result = (*p_droid->bqPlayerPlay)->SetPlayState(p_droid->bqPlayerPlay, SL_PLAYSTATE_STOPPED);
	result = (*p_droid->bqPlayerBufferQueue)->Clear(p_droid->bqPlayerBufferQueue);
	p_droid->running = 0;

	mutex_lock(p_droid->pDataLock);
	p_droid->out_in	= 0;
	p_droid->out_out = 0;
	p_droid->avail = MAX_AUDIO_BLOCK;
	p_droid->push_cnt = 0;

// 	wavebuffer *wb = 0;
// 	for (int i=0;i<MAX_AUDIO_BLOCK;i++){
// 		wb = &p_droid->wavearray[i];
// 	}

	mutex_unlock(p_droid->pDataLock);

	p_droid->bEndOfPcm = 0;
}

//
// Created by dell on 2019/02/26.
//

#ifndef SLPLAYER_SLAUDIO_H
#define SLPLAYER_SLAUDIO_H

#include "SLQueue.h"
#include "PlayStatus.h"
#include "CallJava.h"
#include "SoundTouch.h"
#include "../utils/AndroidLog.h"

using namespace soundtouch;

extern "C"
{
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <libswresample/swresample.h>
#include "libavcodec/avcodec.h"
#include <libavutil/time.h>
};

class SLAudio {

public:
    //=========================FFmpeg相关=========================
    AVCodecContext *avCodecContext = NULL;
    AVPacket *avPacket = NULL;
    AVFrame *avFrame = NULL;
    AVRational time_base;

    //=========================OpenSLES相关=========================
    //引擎 接口对象
    SLObjectItf engineObj = NULL;
    //播放器 接口对象
    SLObjectItf pcmObj = NULL;
    //混音器 接口对象
    SLObjectItf outputMixObj = NULL;

    //引擎
    SLEngineItf engineItf = NULL;
    //缓冲器队列
    SLAndroidSimpleBufferQueueItf pcmBufferQueue = NULL;
    SLPlayItf pcmPlayer = NULL;
    SLVolumeItf pcmVolume = NULL;
    SLMuteSoloItf pcmMute = NULL;

    SLEnvironmentalReverbItf outputMixEnvReverb = NULL;
    SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

    //=========================其他=========================

    PlayStatus *playStatus = NULL;
    SLQueue *queue = NULL;
    CallJava *callJava = NULL;

    int streamIndex = - 1;
    int64_t duration = 0;
    int sample_rate = 0;

    uint8_t *buffer = NULL;
    double clock;//总的播放时长
    double last_time;//上一次调用的时间
    double now_time;//当前frame时间

    pthread_t thread_play;
    pthread_mutex_t codecMutex;

    //=========================其他=========================
    SoundTouch *soundTouch = NULL;
    SAMPLETYPE *sampleBuffer = NULL;
    uint8_t *soundBuffer = NULL;
    int nb = 0;//采样个数
    int num = 0;
    bool soundFinish = false;

public:
    SLAudio(PlayStatus *playStatus, CallJava *callJava, int sample_rate);
    ~SLAudio();

    void initOpenSLES();

    void play();
    int resampleAudio();
    void pause();
    void resume();
    void stop();
    void release();
    void setVolume(int percent);
    void setChannelMute(int type);

    int getSoundTouch();
    void setPitch(float pitch);//设置变调
    void setSpeed(float speed);

private:

    SLuint32 getSampleRateFromSlES(int sample_rate);

};


#endif //SLPLAYER_SLAUDIO_H

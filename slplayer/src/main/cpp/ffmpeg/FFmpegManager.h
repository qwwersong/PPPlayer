//
// Created by dell on 2019/02/20.
//

#ifndef TIKTOKDEMO_FFMPEGMANAGER_H
#define TIKTOKDEMO_FFMPEGMANAGER_H

#include "pthread.h"
#include "../utils/AndroidLog.h"
#include "PlayStatus.h"
#include "SLAudio.h"
#include "SLVideo.h"

extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include <libavutil/time.h>
};


class FFmpegManager {

public:
    AVFormatContext *avFormatContext;
    PlayStatus *playStatus;
    BaseSurface *baseSurface;
    SLAudio *audio = NULL;
    SLVideo *video = NULL;
    CallJava *callJava;
    const char *url;

    pthread_t startThread;
    pthread_t connectThread;
    pthread_mutex_t seekMutex;
    bool decodeFinish = false;

    const AVBitStreamFilter *bsFilter = NULL;

public:
    FFmpegManager(CallJava *callJava);
    ~FFmpegManager();

    void initFFmpeg();
    void connect(const char *url);
    void decode();
    void getStreamInfo();

    //播放器控制相关控制
    void start();
    void pause();
    void resume();
    void release();
    void seek(int64_t sec);

    //音频相关设置
    void setVolume(int percent);
    void setChannelMute(int type);
    void setPitch(float pitch);//设置变调
    void setSpeed(float speed);

private:



};


#endif //TIKTOKDEMO_FFMPEGMANAGER_H

//
// Created by dell on 2019/02/26.
//

#ifndef SLPLAYER_SLVIDEO_H
#define SLPLAYER_SLVIDEO_H

#include "SLQueue.h"
#include "PlayStatus.h"
#include "SLAudio.h"
#include "../utils/AndroidLog.h"
#include "../gles/BaseSurface.h"
#include "../utils/TimeUtil.h"

extern "C"
{
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
};

#define DECODE_SOFT 0
#define DECODE_HARD 1

class SLVideo {

public:
    int streamIndex = -1;//这是干啥用的
    AVCodecContext *avCodecContext = NULL;
    AVCodecParameters *codecpar = NULL;
    BaseSurface *baseSurface = NULL;
    PlayStatus *playStatus = NULL;
    SLQueue *queue = NULL;
    CallJava *callJava = NULL;

    pthread_t thread_play;

    AVRational time_base;
    double clock = 0;
    double defaultDelayTime = 0.04;

    //视频对音频的延时时间
    SLAudio *audio = NULL;
    double delayTime = 0;

    pthread_mutex_t codecMutex;

    int decodeType = DECODE_SOFT;
    AVBSFContext *bsf_ctx = NULL;

public:
    SLVideo(PlayStatus *playStatus, BaseSurface *surface, CallJava *callJava);
    ~SLVideo();

    void play();
    void release();

    void setSurface(BaseSurface *baseSurface);

    void decodeSoft(AVFrame *avFrame);
    void decodeHard(AVPacket *avPacket);

    double getFrameDiffTime(AVFrame *avFrame, AVPacket *avPacket);//视频两帧之间的间隔
    double getDelayTime(double diff);
};


#endif //SLPLAYER_SLVIDEO_H

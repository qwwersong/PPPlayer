//
// Created by dell on 2019/02/26.
//

#ifndef SLPLAYER_SLQUEUE_H
#define SLPLAYER_SLQUEUE_H

#include "queue"
#include "pthread.h"
#include "PlayStatus.h"

extern "C"
{
#include <libavcodec/avcodec.h>
};

class SLQueue {

public:
    std::queue<AVPacket *> queuePacket;
    pthread_mutex_t mutexPacket;
    pthread_cond_t condPacket;

    PlayStatus *playStatus;

public:
    SLQueue(PlayStatus *playStatus);
    ~SLQueue();

    int putAVPacket(AVPacket *packet);
    int getAVPacket(AVPacket *packet);

    int getQueueSize();
    void clearAVPacket();

};


#endif //SLPLAYER_SLQUEUE_H

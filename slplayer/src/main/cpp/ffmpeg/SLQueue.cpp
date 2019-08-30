//
// Created by dell on 2019/02/26.
//

#include "SLQueue.h"

SLQueue::SLQueue(PlayStatus *playStatus) {
    this->playStatus = playStatus;
    pthread_mutex_init(&mutexPacket, NULL);
    pthread_cond_init(&condPacket, NULL);
}

SLQueue::~SLQueue() {
    clearAVPacket();
    pthread_mutex_destroy(&mutexPacket);
    pthread_cond_destroy(&condPacket);
}

int SLQueue::putAVPacket(AVPacket *packet) {
    pthread_mutex_lock(&mutexPacket);

    queuePacket.push(packet);
    pthread_cond_signal(&condPacket);

    pthread_mutex_unlock(&mutexPacket);
    return 0;
}

int SLQueue::getAVPacket(AVPacket *packet) {
    pthread_mutex_lock(&mutexPacket);

//    while (playStatus != NULL && !playStatus->isExit) {
        if (queuePacket.size() > 0) {
            AVPacket *avPacket = queuePacket.front();//取出队列中的avPacket
            if (av_packet_ref(packet, avPacket) == 0) {//把avPacket的内存数据拷贝到packet中，拷贝的是引用，内存并不增加
                queuePacket.pop();
            }
            av_packet_free(&avPacket);//这个地方的释放没太听懂
            av_free(avPacket);
            avPacket = NULL;
//            break;
        } else {
            pthread_cond_wait(&condPacket, &mutexPacket);
        }
//    }

    pthread_mutex_unlock(&mutexPacket);
    return 0;
}

int SLQueue::getQueueSize() {
    pthread_mutex_lock(&mutexPacket);
    int size = queuePacket.size();
    pthread_mutex_unlock(&mutexPacket);
    return size;
}

//清空队列
void SLQueue::clearAVPacket() {
    //有可能getAVPacket在线程锁中等待，
    pthread_cond_signal(&condPacket);
    pthread_mutex_lock(&mutexPacket);

    while (!queuePacket.empty()) {
        AVPacket *packet = queuePacket.front();
        queuePacket.pop();
        av_packet_free(&packet);
        av_free(packet);
        packet = NULL;
    }

    pthread_mutex_unlock(&mutexPacket);
}

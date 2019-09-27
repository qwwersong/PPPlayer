//
// Created by dell on 2019/02/26.
//

#include "SLVideo.h"

SLVideo::SLVideo(PlayStatus *playStatus, BaseSurface *surface, CallJava *callJava) {
    this->playStatus = playStatus;
    this->baseSurface = surface;
    this->callJava = callJava;
    queue = new SLQueue(playStatus);
    pthread_mutex_init(&codecMutex, NULL);
}

SLVideo::~SLVideo() {
    LOGE("SLVideo::~SLVideo()");
    pthread_mutex_destroy(&codecMutex);
}

void SLVideo::setSurface(BaseSurface *surface) {
    this->baseSurface = surface;
}

//avPacket解码获取avFrame
void *playVideo(void *data) {
    LOGE("SLVideo::playVideo")
    SLVideo *slVideo = static_cast<SLVideo *>(data);
    while (slVideo->playStatus != NULL && !slVideo->playStatus->isExit) {

        if (slVideo->playStatus->pasue) {
            //TODO::这里暂停和没有是有区别的，暂停后可能会有好几帧的数据缓慢显示
            //TODO::暂停时这里还在跑
            av_usleep(1000 * 100);
            continue;
        }

        if (slVideo->queue->getQueueSize() == 0) {
            av_usleep(1000 * 100);
            continue;
        }

        pthread_mutex_lock(&slVideo->codecMutex);
        AVPacket *avPacket = av_packet_alloc();
        if (slVideo->queue->getAVPacket(avPacket) != 0) {
            LOGE("playVideo getAVPacket Failed");
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            pthread_mutex_unlock(&slVideo->codecMutex);
            continue;
        }
        if (avcodec_send_packet(slVideo->avCodecContext, avPacket) != 0) {
            LOGE("playVideo avcodec_send_packet Failed");
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            pthread_mutex_unlock(&slVideo->codecMutex);
            continue;
        }
//        AVFrame *avFrame = av_frame_alloc();
//        if (avcodec_receive_frame(slVideo->avCodecContext, avFrame) != 0) {
//            LOGE("playVideo avcodec_receive_frame Failed");
//            av_frame_free(&avFrame);
//            av_free(avFrame);
//            avFrame = NULL;
//            av_packet_free(&avPacket);
//            av_free(avPacket);
//            avPacket = NULL;
//            pthread_mutex_unlock(&slVideo->codecMutex);
//            continue;
//        }

        if (slVideo->decodeType == DECODE_HARD) {
            //硬解码
            if (av_bsf_send_packet(slVideo->bsf_ctx, avPacket) != 0) {
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
                pthread_mutex_unlock(&slVideo->codecMutex);
                continue;
            }
            while (av_bsf_receive_packet(slVideo->bsf_ctx, avPacket) == 0) {
                LOGE("开始硬解码");
                double diff = slVideo->getFrameDiffTime(NULL, avPacket);
                av_usleep(slVideo->getDelayTime(diff) * 1000000);

                slVideo->callJava->onCallDecodeAVPacket(avPacket->size, avPacket->data);

                av_packet_free(&avPacket);
                av_free(avPacket);
                continue;
            }
            avPacket = NULL;
            slVideo->decodeHard(avPacket);
        } else if (slVideo->decodeType == DECODE_SOFT) {
            AVFrame *avFrame = av_frame_alloc();
            if (avcodec_receive_frame(slVideo->avCodecContext, avFrame) != 0) {
                LOGE("playVideo avcodec_receive_frame Failed");
                av_frame_free(&avFrame);
                av_free(avFrame);
                avFrame = NULL;
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
                pthread_mutex_unlock(&slVideo->codecMutex);
                continue;
            }
            //软解码
            slVideo->decodeSoft(avFrame);
        }

        //必须要释放，不释放的话占用内存越来越高，会越来越卡
//        av_frame_free(&avFrame);
//        av_free(avFrame);
//        avFrame = NULL;
        av_packet_free(&avPacket);
        av_free(avPacket);
        avPacket = NULL;
        pthread_mutex_unlock(&slVideo->codecMutex);
    }
    pthread_exit(&slVideo->thread_play);
}

//硬解码
void SLVideo::decodeHard(AVPacket *avPacket) {
    if (av_bsf_send_packet(bsf_ctx, avPacket) != 0) {
        av_packet_free(&avPacket);
        av_free(avPacket);
        avPacket = NULL;
        pthread_mutex_unlock(&codecMutex);
        return;
    }
    while (av_bsf_receive_packet(bsf_ctx, avPacket) == 0) {
        LOGE("开始硬解码");
        double diff = getFrameDiffTime(NULL, avPacket);
        av_usleep(getDelayTime(diff) * 1000000);

        callJava->onCallDecodeAVPacket(avPacket->size, avPacket->data);

        av_packet_free(&avPacket);
        av_free(avPacket);
        continue;
    }
    avPacket = NULL;
}

//软解码
void SLVideo::decodeSoft(AVFrame *avFrame) {
    if (avFrame->format == AV_PIX_FMT_YUV420P) {//是YUV420P格式"
        // avFrame->key_frame判断是否是I帧
        double diff = getFrameDiffTime(avFrame, NULL);
        LOGE("decodeSoft diff = %f", diff)
        double delayTime = getDelayTime(diff) * 1000000;
//        LOGE("最后睡眠时间 = %f", delayTime);
        av_usleep(delayTime);
        baseSurface->drawFrame(avCodecContext, avFrame);
    } else {//当前视频不是YUV420P格式
        LOGE("decodeSoft 重采样");
        //这里也相当于重采样
        AVFrame *pFrameYUV = av_frame_alloc();
        int num = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
                                           avCodecContext->width,
                                           avCodecContext->height, 1);
        uint8_t *buffer = static_cast<uint8_t *>(av_malloc(num * sizeof(uint8_t)));
        //将解码获取的avframe获取的yuv数据复制给buffer
        av_image_fill_arrays(
                pFrameYUV->data,
                pFrameYUV->linesize,
                buffer,
                AV_PIX_FMT_YUV420P,
                avCodecContext->width,
                avCodecContext->height,
                1
        );
        //创建重采样上下文
        SwsContext *sws_ctx = sws_getContext(
                avCodecContext->width,
                avCodecContext->height,
                avCodecContext->pix_fmt,//视频源的视频格式
                avCodecContext->width,
                avCodecContext->height,
                AV_PIX_FMT_YUV420P,
                SWS_BICUBIC, NULL, NULL, NULL
        );
        //创建失败
        if (!sws_ctx) {
            av_frame_free(&pFrameYUV);
            av_free(pFrameYUV);
            av_free(buffer);
            pthread_mutex_unlock(&codecMutex);
            return;
        }
        //转化
        sws_scale(
                sws_ctx,
                (const uint8_t *const *)(avFrame->data),
                avFrame->linesize,
                0,
                avFrame->height,
                pFrameYUV->data,
                pFrameYUV->linesize
        );

        double diff = getFrameDiffTime(avFrame, NULL);
        av_usleep(getDelayTime(diff) * 1000000);

        baseSurface->drawFrame(avCodecContext, pFrameYUV);

        av_free(buffer);
        sws_freeContext(sws_ctx);
    }
}

void SLVideo::play() {
    pthread_create(&thread_play, NULL, playVideo, this);
}

void SLVideo::release() {
    LOGE("SLVideo::release()");
    pthread_join(thread_play, NULL);
    //停止播放时，要释放掉内存
    //这里如果清空queue数据，正好在playVideo时sleep，其他数据都释放完了，这是sleep完后再画画会崩溃
    if (queue != NULL) {
        delete(queue);//调用delete就会调用queue的析构函数
        queue = NULL;
    }
    if (baseSurface != NULL) {
        delete(baseSurface);
        baseSurface = NULL;
    }
    if (callJava != NULL) {
        callJava = NULL;
    }
    //这里必须加锁，因为queue中有缓冲数据，video线程还在循环会获取数据并绘制
    if (avCodecContext != NULL) {
        pthread_mutex_lock(&codecMutex);
        avcodec_close(avCodecContext);
        avcodec_free_context(&avCodecContext);
        avCodecContext = NULL;
        pthread_mutex_unlock(&codecMutex);
    }

    //playstatus和calljava只需清空引用，他的对象再ffmpeg对象中还在应用
    if (playStatus != NULL) {
        playStatus = NULL;
    }
    if (audio != NULL) {
        audio = NULL;
    }
}

double SLVideo::getFrameDiffTime(AVFrame *avFrame, AVPacket *avPacket) {
    double pts = 0;
    if (avFrame != NULL) {
        pts = av_frame_get_best_effort_timestamp(avFrame);
        LOGE("getFrameDiffTime 获取avFrame的pts = %f", pts)
    }
    if (avPacket != NULL) {
        pts = avPacket->pts;
        LOGE("getFrameDiffTime 获取avPacket的pts = %f", pts)
    }
    //判断是否有有效的pts，这个pts不一定有值
    if (pts == AV_NOPTS_VALUE) {
        pts = 0;
    }
    pts *= av_q2d(time_base);//为什么要选取流的time_base而不选别的
    LOGE("getFrameDiffTime av_q2d pts = %f", pts)

    if (pts > 0) {//如果不大于零，这个clock就是上一帧视频的clock时间
        clock = pts;
    }

    LOGE("getFrameDiffTime 音频audio的clock = %f pts的clock = %f", audio->clock, clock)
    //音频当前帧的时间戳-视频当前的时间戳
    double diff = audio->clock - clock;//>0的话就是视频慢，音频快放完了，要让视频睡眠小一点
    return diff;
}

//音视频同步
double SLVideo::getDelayTime(double diff) {
    //这个地方研究一下，加注释
    //单位是秒
    /**
     * diff是上面计算得到，是两帧之间音频时间差
     */
    LOGE("getDelayTime diff = %f  delayTime = %f", diff, delayTime)
    //TODO::这个策略有问题，diff通过上面getFrameDiffTime方法计算获得，第一帧是0.09，最后delayTime计算得到0.027
    //TODO::逐渐相加越来越大，
    if (diff > 0.003) {//视频慢 3ms
        delayTime = delayTime * 2 / 3;//休眠时间要短一点
        if (delayTime < defaultDelayTime / 2) {
            delayTime = defaultDelayTime * 2 / 3;
        } else if (delayTime > defaultDelayTime * 2) {
            delayTime = defaultDelayTime * 2;
        }
    } else if (diff < -0.003) {//视频快
        delayTime = delayTime * 3 / 2;
        if (delayTime < defaultDelayTime / 2) {
            delayTime = defaultDelayTime * 2 / 3;
        } else if (delayTime > defaultDelayTime * 2) {
            delayTime = defaultDelayTime * 2;
        }
    } else if (diff == 0.003) {

    }
    if (diff >= 0.5) {//视频比音频慢
        delayTime = 0;
    } else if (diff <= -0.5) {//音频比视频快
        delayTime = defaultDelayTime * 2;
    }

    if (fabs(diff) >= 10) {
        delayTime = defaultDelayTime;
    }
    LOGE("最后延时时间 = %f", delayTime);
//    return 0;
    return delayTime;
}


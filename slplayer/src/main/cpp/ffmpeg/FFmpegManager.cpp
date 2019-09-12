//
// Created by dell on 2019/02/20.
//

#include "FFmpegManager.h"
#include "include/libavcodec/avcodec.h"

FFmpegManager::FFmpegManager(CallJava *callJava) {
//    this->url = url;
    this->callJava = callJava;
    playStatus = new PlayStatus();
    pthread_mutex_init(&seekMutex, NULL);
    decodeFinish = false;
}

FFmpegManager::~FFmpegManager() {
    pthread_mutex_destroy(&seekMutex);
}

int avformat_callback(void *data) {
    FFmpegManager *ffmpegManager = static_cast<FFmpegManager *>(data);
    if (ffmpegManager->playStatus->isExit) {
        return AVERROR_EOF;
    }
    return 0;
}

void FFmpegManager::setSurface(BaseSurface *surface) {
    if (video != NULL) {
        LOGE("设置surface");
        video->setSurface(surface);
    }
}

void FFmpegManager::initFFmpeg() {
    //1、注册
    av_register_all();
    //2、网络初始化
    avformat_network_init();
    //3、创建avFormatContext
    avFormatContext = avformat_alloc_context();

    //这个回调对应的是avformat_open_input连接网络时间比较长的情况
    avFormatContext->interrupt_callback.callback = avformat_callback;
    avFormatContext->interrupt_callback.opaque = this;
}

void *connectCallBack(void *data){
    FFmpegManager *ffmpegManager = static_cast<FFmpegManager *>(data);
    ffmpegManager->getStreamInfo();
    return 0;
}

void FFmpegManager::connect(const char *url) {
    this->url = url;
    pthread_create(&connectThread, NULL, connectCallBack, this);
}

AVCodecContext *getAVCodecContext(AVCodecParameters *codecpar) {
    //根据流中的编码信息，获取相应的解码器
    LOGE("获取解码器信息");
    AVCodec *avCodec = avcodec_find_decoder(codecpar->codec_id);
    if (!avCodec) {
        LOGE("can't find decoder");
        return NULL;
    }
    //分配解码器上下文空间
    AVCodecContext *avCodecContext = avcodec_alloc_context3(avCodec);
    if (!avCodecContext) {
        LOGE("can't alloc avcontext");
        return NULL;
    }
    //将相应流中的参数赋值给解码器上下文
    if (avcodec_parameters_to_context(avCodecContext, codecpar) < 0) {
        LOGE("can't fill the value from the params");
        return NULL;
    }
    //初始化解码器上下文
    if (avcodec_open2(avCodecContext, avCodec, 0) != 0) {
        LOGE("Initialize avCodecContext failed!");
        return NULL;
    }
    return avCodecContext;
}

void FFmpegManager::getStreamInfo() {
    //4、打开流
    LOGE("open url = %s", url);
    if (avformat_open_input(&avFormatContext, url, NULL, NULL) != 0) {
        //读取url失败
        LOGE("open input failed!");
        return;
    }
    //5、获取流信息
    if (avformat_find_stream_info(avFormatContext, NULL) < 0) {
        //获取流信息失败
        LOGE("get stream info failed!");
        return;
    }
    LOGE("获取流信息");
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        if (avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            //获取流的视频信息
            if (video == NULL) {
                LOGE("获取Video信息 ===========================");
                video = new SLVideo(playStatus, baseSurface, callJava);
                video->streamIndex = i;
                video->codecpar = avFormatContext->streams[i]->codecpar;
                video->time_base = avFormatContext->streams[i]->time_base;

                int num = avFormatContext->streams[i]->avg_frame_rate.num;
                int den = avFormatContext->streams[i]->avg_frame_rate.den;
                if (num != 0 && den != 0) {
                    LOGE("num = %d, den = %d", num, den);
                    int fps = num / den;
                    video->defaultDelayTime = 1.0 / fps;
                    LOGE("defaultDelayTime = %lf", video->defaultDelayTime);
                }
                video->avCodecContext = getAVCodecContext(avFormatContext->streams[i]->codecpar);
            }
        } else if (avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            //获取流的音频信息
            if (audio == NULL) {
                LOGE("获取Audio信息 ===========================");
                audio = new SLAudio(playStatus, callJava, avFormatContext->streams[i]->codecpar->sample_rate);
                audio->streamIndex = i;
                audio->time_base = avFormatContext->streams[i]->time_base;
                LOGE(" audio->time_base->num = %d, time_base->den = %d",  audio->time_base.num, audio->time_base.den);
                audio->duration = avFormatContext->duration / AV_TIME_BASE;
                LOGE(" audio->duration = %d",  audio->duration);

                audio->avCodecContext = getAVCodecContext(avFormatContext->streams[i]->codecpar);
            }
        }
    }
    LOGE("FFmpeg信息初始化成功")
    if (callJava != NULL) {
        callJava->onCallPrepare(CHILD_THREAD);
    } else {
        LOGE("callJava = NULL")
    }
}

void FFmpegManager::decode() {
    pthread_join(connectThread, NULL);
    //从解码器中获取到avPacket，并放入队列中
    if (audio == NULL || video == NULL) {
        LOGE("audio or video = NULL")
        return;
    }
    LOGE("FFmpegManager 播放");
    video->audio = audio;

    const char *codecName = ((const AVCodec*)video->avCodecContext->codec)->name;
    LOGE("该视频的解码器 codecName = %s", codecName);
//    bool isSupportMediaDecode = callJava->onCallSupportDecode(codecName);
//    LOGE("isSupport hard Decode = %s", isSupportMediaDecode);
    bool isSupportMediaDecode = false;
    if (isSupportMediaDecode) {
        //找到相应解码器的过滤器
        if (strcasecmp(codecName, "h264") == 0) {//比较函数
            bsFilter = av_bsf_get_by_name("h264_mp4toannexb");
        } else if (strcasecmp(codecName, "h265") == 0) {
            bsFilter = av_bsf_get_by_name("hevc_mp4toannexb");
        }
        if (bsFilter == NULL) {
            goto end;
        }

        //给AVBSFContext分配空间
        if (av_bsf_alloc(bsFilter, &video->bsf_ctx) != 0) {
            goto end;
        }

        //将解码器属性复制给过滤器
        if (avcodec_parameters_copy(video->bsf_ctx->par_in, video->codecpar) < 0) {
            av_bsf_free(&video->bsf_ctx);
            video->bsf_ctx = NULL;
            goto end;
        }

        //初始化过滤器上下文
        if (av_bsf_init(video->bsf_ctx) != 0) {
            av_bsf_free(&video->bsf_ctx);
            video->bsf_ctx = NULL;
            goto end;
        }

        video->bsf_ctx->time_base_in = video->time_base;
        video->decodeType = DECODE_HARD;
        callJava->onCallInitMediaCodec(
                codecName,
                video->avCodecContext->width,
                video->avCodecContext->height,
                video->avCodecContext->extradata_size,//csd-0的size
                video->avCodecContext->extradata_size,//csd-1的size
                video->avCodecContext->extradata,//csd-0的data
                video->avCodecContext->extradata//csd-1的data
        );
    }

    end:

    audio->play();//音频播放线程
    video->play();//视频播放线程

    //解码器解码音视频线程（包含上面的播放线程）
    while (playStatus != NULL && !playStatus->isExit) {
        if (audio == NULL || audio->queue == NULL) {
            LOGE("play audio = NULL");
            break;
        }
        if (audio->queue->getQueueSize() > 40) {
            av_usleep(1000 * 100);
            continue;
        }
        AVPacket *avPacket = av_packet_alloc();
        //从解码器里获取avPacket数据
        if (av_read_frame(avFormatContext, avPacket) == 0) {
            // 读取成功
            if (avPacket->stream_index == video->streamIndex) {
//                LOGE("FFmpegManager Video帧放入队列。。。");
                video->queue->putAVPacket(avPacket);
            } else if (avPacket->stream_index == audio->streamIndex) {
//                LOGE("FFmpegManager Audio帧放入队列。。。");
                audio->queue->putAVPacket(avPacket);
            } else {
                LOGE("FFmpegManager 读取失败");
                av_packet_free(&avPacket);
                av_free(avPacket);
            }
        } else {
            // < 0 on error or end of file
            LOGE("FFmpegManager 结束了 解码完成了？");
            av_packet_free(&avPacket);
            av_free(avPacket);
            while (playStatus != NULL && !playStatus->isExit) {
                LOGE("FFmpegManager 结束了 queue size = %d", video->queue->getQueueSize());
                if (video->queue->getQueueSize() > 0) {
                    av_usleep(1000 * 100);
                    continue;
                } else {
                    LOGE("FFmpegManager 结束了 isExit = true");
                    playStatus->isExit = true;
                    break;
                }
            }
            break;
        }
    }

    if (callJava != NULL) {
        callJava->onCallComplete(CHILD_THREAD);
    } else {
        LOGE("callJava = NULL")
    }
    decodeFinish = true;
    LOGE("解码完成");
}

void *startCallBack(void *data){
    FFmpegManager *ffmpegManager = static_cast<FFmpegManager *>(data);
    ffmpegManager->decode();
    pthread_exit(&ffmpegManager->startThread);
}

void FFmpegManager::start() {
    pthread_create(&startThread, NULL, startCallBack, this);
}

void FFmpegManager::pause() {
    if (audio != NULL) {
        audio->pause();
    }
}

void FFmpegManager::resume() {
    if (audio != NULL) {
        audio->resume();
    }
}

void FFmpegManager::release() {
    LOGE("FFmpegManager release");
    playStatus->isExit = true;

    //只有解码线程完全退出后，才释放其他对象，要不然释放完对象后
    //解码线程运行到最后是calljava为空，具体原因不知道，可能这个对象被回收后，里面的变量也被回收了？
    while (!decodeFinish) {
        av_usleep(1000 * 10);//暂停10毫秒
    }

    LOGE("释放audio");
    if (audio != NULL) {
        audio->release();
        delete(audio);
        audio = NULL;
    }
    LOGE("释放video");
    if (video != NULL) {
        video->release();
        delete(video);
        video = NULL;
    }
    LOGE("释放avFormatContext");
    if (avFormatContext != NULL) {
        avformat_close_input(&avFormatContext);
        avformat_free_context(avFormatContext);
        avFormatContext = NULL;
    }
    LOGE("释放playStatus");
    if (playStatus != NULL) {
        delete(playStatus);
        playStatus = NULL;
    }

//    if (callJava != NULL) {
//        callJava = NULL;
//    }
    LOGE("释放完成");
}

void FFmpegManager::seek(int64_t sec) {
    //sec用int64_t是因为可能是一个很大的值，超过了int大小
    if (audio->duration <= 0) {
        return;
    }

    if (sec >= 0 && sec <= audio->duration) {
        //这个seekMutex只在这个seek方法中加锁了，这时其他地方不会调用么？
        pthread_mutex_lock(&seekMutex);

        //实际的seek时间，要乘以时间基
        int64_t relTime = sec * AV_TIME_BASE;
        avformat_seek_file(avFormatContext, -1, INT64_MIN, relTime, INT64_MAX, 0);

        if (audio != NULL) {
            //里面有缓存，所以要清除
            audio->queue->clearAVPacket();
            audio->clock = 0;
            audio->last_time = 0;
            //这个地方加锁是防止audio和video解码的时候，没有数据而出现异常
            pthread_mutex_lock(&audio->codecMutex);
            avcodec_flush_buffers(audio->avCodecContext);//这个是干啥的
            pthread_mutex_unlock(&audio->codecMutex);
        }

        if (video != NULL) {
            video->queue->clearAVPacket();
            video->clock = 0;
            pthread_mutex_lock(&video->codecMutex);
            avcodec_flush_buffers(video->avCodecContext);
            pthread_mutex_unlock(&video->codecMutex);
        }

        pthread_mutex_unlock(&seekMutex);
    }
}

void FFmpegManager::setVolume(int percent) {
    if (audio != NULL) {
        audio->setVolume(percent);
    }
}

void FFmpegManager::setChannelMute(int type) {
    if (audio != NULL) {
        audio->setChannelMute(type);
    }
}

void FFmpegManager::setPitch(float pitch) {
    if (audio != NULL) {
        audio->setPitch(pitch);
    }
}

void FFmpegManager::setSpeed(float speed) {
    if (audio != NULL) {
        audio->setSpeed(speed);
    }
}




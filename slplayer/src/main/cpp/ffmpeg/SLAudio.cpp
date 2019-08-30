//
// Created by dell on 2019/02/26.
//

#include "SLAudio.h"

SLAudio::SLAudio(PlayStatus *playStatus, CallJava *callJava, int sample_rate) {
    this->playStatus = playStatus;
    this->callJava = callJava;
    this->sample_rate = sample_rate;
    queue = new SLQueue(playStatus);
    buffer = (uint8_t *) av_malloc(sample_rate * 2 * 2);
    pthread_mutex_init(&codecMutex, NULL);

    // 声道数：2，每个采样点占用的内存大小：16位/8位（每8位=1bit）= 2bit
    //soundtouch
    sampleBuffer = static_cast<SAMPLETYPE *>(malloc(sample_rate * 2 * 2));
    soundTouch = new SoundTouch();
    soundTouch->setSampleRate(sample_rate);
    soundTouch->setChannels(2);
    soundTouch->setPitch(1.0f);
    soundTouch->setTempo(1.0f);
}

SLAudio::~SLAudio() {
    pthread_mutex_destroy(&codecMutex);
}

void *decodeData(void *data) {
    SLAudio *audio = static_cast<SLAudio *>(data);
    audio->initOpenSLES();//initOpenSLES必须public才能调用
    pthread_exit(&audio->thread_play);
}

void SLAudio::play() {
    pthread_create(&thread_play, NULL, decodeData, this);
}

void pcmBufferCallBack(SLAndroidSimpleBufferQueueItf bf, void *context) {//循环回调该函数
//    LOGE("SLAudio::pcmBufferCallBack")
    SLAudio *audio = static_cast<SLAudio *>(context);
    if (audio != NULL) {
        int bufferSize = audio->getSoundTouch();
        if (bufferSize > 0) {
            // time = bufferSize / audio->sample_rate * 2 * 2; 这个time表示是pcm理论上播放的时间
            // 这个clock就是理论上每个AVframe播放时间 + 这个pcm播放所消耗的时间。
//            LOGE("解码得到一个Audio数据");
            audio->clock += bufferSize / (double) (audio->sample_rate * 2 * 2);
            //1、这个地方为什么是0.1？因为有可能会回调很多次，如果大于这个0.1秒阈值才会回调
            //2、这个clock的时间是否合理准确？
            if (audio->clock - audio->last_time >= 0.1) {
                audio->last_time = audio->clock;
//                LOGE("onCallTimeInfo clock = %lf duration = %d", audio->clock, audio->duration);
                audio->callJava->onCallTimeInfo(CHILD_THREAD, audio->clock, audio->duration);
            }
            //循环入队，放入到这个队列后，OpenSLES会循环从这个队列取数据，并将数据放到mic播放
            //这里用soundTouch后为什么就变为sampleBuffer，bufferSize也变了
            (*audio->pcmBufferQueue)->Enqueue(audio->pcmBufferQueue, (char *) audio->sampleBuffer, bufferSize * 2 * 2);
        }
    }
}

void SLAudio::initOpenSLES() {
    LOGE("SLAudio::initOpenSLES")
    //第一步，创建引擎
    SLresult result;
    result = slCreateEngine(&engineObj, 0, 0, 0, 0, 0);
    result = (*engineObj)->Realize(engineObj, SL_BOOLEAN_FALSE);
    result = (*engineObj)->GetInterface(engineObj, SL_IID_ENGINE, &engineItf);

    //第二步，创建混音器
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};

    result = (*engineItf)->CreateOutputMix(engineItf, &outputMixObj, 1, mids, mreq);
//    (void)result;//干啥的？
    result = (*outputMixObj)->Realize(outputMixObj, SL_BOOLEAN_FALSE);
//    (void)result;
    result = (*outputMixObj)->GetInterface(outputMixObj, SL_IID_ENVIRONMENTALREVERB, &outputMixEnvReverb);

    if (result == SL_RESULT_SUCCESS) {
        result = (*outputMixEnvReverb)->SetEnvironmentalReverbProperties(outputMixEnvReverb, &reverbSettings);
//        (void)result;
        LOGE("create OpenSL Success");
    } else {
        LOGE("create OpenSL Failed");
        return;
    }

    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObj};
    SLDataSink audioSnk = {&outputMix, 0};

    //第三步，配置PCM格式信息
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM pcmFormat = {
            SL_DATAFORMAT_PCM,//播放pcm格式的数据
            2,//2个声道（立体声）
            getSampleRateFromSlES(sample_rate),//获取频率
            SL_PCMSAMPLEFORMAT_FIXED_16,//位数 16位
            SL_PCMSAMPLEFORMAT_FIXED_16,//和位数一致就行，为什么？
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//立体声（前左前右）
            SL_BYTEORDER_LITTLEENDIAN//结束标志
    };
    SLDataSource slDataSource = {&android_queue, &pcmFormat};
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_MUTESOLO};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    //第四步，创建播放器
    (*engineItf)->CreateAudioPlayer(engineItf, &pcmObj, &slDataSource, &audioSnk, 3, ids, req);
    //初始化播放器
    (*pcmObj)->Realize(pcmObj, SL_BOOLEAN_FALSE);

    //得到接口后调用，获取Player接口
    (*pcmObj)->GetInterface(pcmObj, SL_IID_PLAY, &pcmPlayer);
    //获取音量接口
    (*pcmObj)->GetInterface(pcmObj, SL_IID_VOLUME, &pcmVolume);
    //获取声道接口
    (*pcmObj)->GetInterface(pcmObj, SL_IID_MUTESOLO, &pcmMute);

    //获取缓冲队列接口
    (*pcmObj)->GetInterface(pcmObj, SL_IID_BUFFERQUEUE, &pcmBufferQueue);
    //注册回调
    (*pcmBufferQueue)->RegisterCallback(pcmBufferQueue, pcmBufferCallBack, this);
    //获取播放状态
    (*pcmPlayer)->SetPlayState(pcmPlayer, SL_PLAYSTATE_PLAYING);
    pcmBufferCallBack(pcmBufferQueue, this);
}

int SLAudio::resampleAudio(void **pcmBuffer) {
//    LOGE("SLAudio::resampleAudio")
    int data_size = 0;

    while (playStatus != NULL && !playStatus->isExit) {
        if (queue->getQueueSize() == 0) {
            //队列中没有数据了，sleep一下，加载状态
            av_usleep(1000 * 100);
            continue;
        }
//        LOGE("SLAudio::resampleAudio1")
        pthread_mutex_lock(&codecMutex);
        avPacket = av_packet_alloc();
        //从缓冲队列中获取avPacket
        if (queue->getAVPacket(avPacket) != 0) {
            LOGE("playAudio getAVPacket Failed");
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            pthread_mutex_unlock(&codecMutex);
            continue;
        }
        //把avPacket数据放到解码器里进行解码
        if (avcodec_send_packet(avCodecContext, avPacket) != 0) {
            LOGE("playAudio avcodec_send_packet Failed");
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            pthread_mutex_unlock(&codecMutex);
            continue;
        }
        avFrame = av_frame_alloc();
        //接收avFrame数据
        if (avcodec_receive_frame(avCodecContext, avFrame) != 0) {
            LOGE("playAudio avcodec_receive_frame Failed");
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            pthread_mutex_unlock(&codecMutex);
            continue;
        }

        //===============================重采样===============================
        //把目标音频不同格式重新采样编码成新的统一格式音频
        //目前输入和输出的采样率是相同的，不能修改，如果要修改跟AVFilter有关

        //channels声道数、channel_layout声道布局
        //根据声道数获取声道布局，或者根据声道布局获取声道数
        if (avFrame->channels && avFrame->channel_layout == 0) {
            avFrame->channel_layout = av_get_default_channel_layout(avFrame->channels);
        } else if (avFrame->channels == 0 && avFrame->channel_layout > 0) {
            avFrame->channels = av_get_channel_layout_nb_channels(avFrame->channel_layout);
        }

        //重采样上下文
        SwrContext *swr_ctx;
        swr_ctx = swr_alloc_set_opts(
                NULL,
                AV_CH_LAYOUT_STEREO,//输出的声道布局
                AV_SAMPLE_FMT_S16,//输出采样位数格式
                avFrame->sample_rate,//输出采样率
                avFrame->channel_layout,//输入声道布局
                (AVSampleFormat) avFrame->format,//输入采样位数格式
                avFrame->sample_rate,//输入采样率
                NULL, NULL//这两个log相关
        );
        if (!swr_ctx || swr_init(swr_ctx) < 0) {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            swr_free(&swr_ctx);
            pthread_mutex_unlock(&codecMutex);
            continue;
        }

        //计算PCM数据大小 size = 采样个数 * 声道数 * 单个采样点大小
        //44100Hz、16bit、2个声道  size = 44100 * 2 * (16/8)这是一秒钟的PCM数据大小，16/8=2 为两个字节
        //实际调用 返回值nb为输出采样个数，理论上和avFrame->nb_samples是一样的
        nb = swr_convert(
                swr_ctx,
                &buffer,//转码后输出的PCM数据
                avFrame->nb_samples,//输出采样个数，实际处理的数据个数，而不是1秒采样个数=采样率
                (const uint8_t **) avFrame->data,//输出的avFrame中原始压缩数据
                avFrame->nb_samples);//输入采样个数

        int out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
        //采样个数 * 声道数 * 单个采样点大小
        data_size = nb * out_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

        //avFrame->pts （int64_t类型的，当前帧展示给用户的时间戳，基于time_base，这个time_base是分数形式，类似于帧率）
        //av_q2d方法就是time_base.num / (double)time_base.den
        //这个clock是理论上视频播放时间。
        now_time = avFrame->pts * av_q2d(time_base);
        if (now_time < clock) {
            now_time = clock;
        }
        clock = now_time;
        //ffmpeg解码的pcm数据是8bit，将这个数据传给外面
        *pcmBuffer = buffer;

        av_packet_free(&avPacket);
        av_free(avPacket);
        avPacket = NULL;
        av_frame_free(&avFrame);
        av_free(avFrame);
        avFrame = NULL;
        swr_free(&swr_ctx);
        pthread_mutex_unlock(&codecMutex);
        break;
    }
    return data_size;
}

int SLAudio::getSoundTouch() {
    while (playStatus != NULL && !playStatus->isExit) {
        soundBuffer = NULL;
        int data_size = 0;

        //这个地方的流程、api还是不是太懂
        if (soundFinish) {//判断receiveSamples是否完成
            soundFinish = false;
            data_size = resampleAudio(reinterpret_cast<void **>(&soundBuffer));
            if (data_size > 0) {
                for (int i = 0; i < data_size / 2 + 1; ++i) {
                    sampleBuffer[i] = (soundBuffer[i * 2] | (soundBuffer[i * 2 + 1] << 8));
                }
                //nb为采样个数
                soundTouch->putSamples(sampleBuffer, nb);
                num = soundTouch->receiveSamples(sampleBuffer, data_size / 4);
            } else {
                soundTouch->flush();
            }
        }

        if (num == 0) {
            soundFinish = true;
            continue;
        } else {
            if (soundBuffer == NULL) {
                num = soundTouch->receiveSamples(sampleBuffer, data_size / 4);
                if (num == 0) {
                    soundFinish = true;
                    continue;
                }
            }
            return num;
        }
    }
    return 0;
}

SLuint32 SLAudio::getSampleRateFromSlES(int sample_rate) {
    SLuint32 rate = 0;
    switch (sample_rate) {
        case 8000:
            rate = SL_SAMPLINGRATE_8;
            break;
        case 11025:
            rate = SL_SAMPLINGRATE_11_025;
            break;
        case 12000:
            rate = SL_SAMPLINGRATE_12;
            break;
        case 16000:
            rate = SL_SAMPLINGRATE_16;
            break;
        case 22050:
            rate = SL_SAMPLINGRATE_22_05;
            break;
        case 24000:
            rate = SL_SAMPLINGRATE_24;
            break;
        case 32000:
            rate = SL_SAMPLINGRATE_32;
            break;
        case 44100:
            rate = SL_SAMPLINGRATE_44_1;
            break;
        case 48000:
            rate = SL_SAMPLINGRATE_48;
            break;
        case 64000:
            rate = SL_SAMPLINGRATE_64;
            break;
        case 88200:
            rate = SL_SAMPLINGRATE_88_2;
            break;
        case 96000:
            rate = SL_SAMPLINGRATE_96;
            break;
        case 192000:
            rate = SL_SAMPLINGRATE_192;
            break;
        default:
            rate = SL_SAMPLINGRATE_44_1;
    }
    return rate;
}

void SLAudio::pause() {
    if (pcmPlayer != NULL) {
        (*pcmPlayer)->SetPlayState(pcmPlayer, SL_PLAYSTATE_PAUSED);
    }
}

void SLAudio::resume() {
    if (pcmPlayer != NULL) {
        (*pcmPlayer)->SetPlayState(pcmPlayer, SL_PLAYSTATE_PLAYING);
    }
}

void SLAudio::stop() {
    if (pcmPlayer != NULL) {
        (*pcmPlayer)->SetPlayState(pcmPlayer, SL_PLAYSTATE_STOPPED);
    }
}

void SLAudio::release() {
    LOGE("SLAudio::release")
    stop();
    //停止播放时，要释放掉内存
    if (queue != NULL) {
        delete (queue);//调用delete就会调用queue的析构函数
        queue = NULL;
    }
    //释放播放器内存
    if (pcmObj != NULL) {
        (*pcmObj)->Destroy(pcmObj);
        pcmObj = NULL;
        pcmPlayer = NULL;
        pcmBufferQueue = NULL;
    }
    //释放混音器内存
    if (outputMixObj != NULL) {
        (*outputMixObj)->Destroy(outputMixObj);
        outputMixObj = NULL;
        outputMixEnvReverb = NULL;
    }
    //引擎内存释放
    if (engineObj != NULL) {
        (*engineObj)->Destroy(engineObj);
        engineObj = NULL;
        engineItf = NULL;
    }
    //pcm buffer释放
    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }
    //avCodecContext是外部传进来的，也需要在Audio内部用完后释放
    if (avCodecContext != NULL) {
        avcodec_close(avCodecContext);
        avcodec_free_context(&avCodecContext);
        avCodecContext = NULL;
    }
    //playstatus和calljava只需清空引用，他的对象再ffmpeg对象中还在应用
    if (playStatus != NULL) {
        playStatus = NULL;
    }
//    if (callJava != NULL) {
//        callJava = NULL;
//    }
    //那么问题avCodecContext在外部也会被别人引用，为什么不是只是删除引用而已
    LOGE("SLAudio::release完成")
}

void SLAudio::setVolume(int percent) {
    if (pcmVolume != NULL) {
        if (percent > 30) {
            (*pcmVolume)->SetVolumeLevel(pcmVolume, (100 - percent) * -20);
        } else if (percent > 25) {
            (*pcmVolume)->SetVolumeLevel(pcmVolume, (100 - percent) * -22);
        } else if (percent > 20) {
            (*pcmVolume)->SetVolumeLevel(pcmVolume, (100 - percent) * -25);
        } else if (percent > 15) {
            (*pcmVolume)->SetVolumeLevel(pcmVolume, (100 - percent) * -28);
        } else if (percent > 10) {
            (*pcmVolume)->SetVolumeLevel(pcmVolume, (100 - percent) * -30);
        } else if (percent > 5) {
            (*pcmVolume)->SetVolumeLevel(pcmVolume, (100 - percent) * -34);
        } else if (percent > 3) {
            (*pcmVolume)->SetVolumeLevel(pcmVolume, (100 - percent) * -37);
        } else if (percent > 0) {
            (*pcmVolume)->SetVolumeLevel(pcmVolume, (100 - percent) * -40);
        } else {
            (*pcmVolume)->SetVolumeLevel(pcmVolume, (100 - percent) * -100);
        }
    }
}

void SLAudio::setChannelMute(int type) {
    //  0：右声道，1：左声道
    //  false声道关闭，双声道：左右声道都关闭
    if (pcmMute != NULL) {
        if (type == 0) {//切换左声道
            (*pcmMute)->SetChannelMute(pcmMute, 1, false);
            (*pcmMute)->SetChannelMute(pcmMute, 0, true);
        } else if (type == 1) {//切换右声道
            (*pcmMute)->SetChannelMute(pcmMute, 1, true);
            (*pcmMute)->SetChannelMute(pcmMute, 0, false);
        } else if (type == 2) {//切换双声道
            (*pcmMute)->SetChannelMute(pcmMute, 1, false);
            (*pcmMute)->SetChannelMute(pcmMute, 0, false);
        }
    }
}

void SLAudio::setPitch(float pitch) {
    if (soundTouch != NULL) {
        soundTouch->setPitch(pitch);
    }
}

void SLAudio::setSpeed(float speed) {
    if (soundTouch != NULL) {
        soundTouch->setTempo(speed);
    }
}




//
// Created by dell on 2019/03/08.
//

#ifndef SLPLAYER_CALLJAVA_H
#define SLPLAYER_CALLJAVA_H

#include <cwchar>
#include "jni.h"
#include "../utils/AndroidLog.h"

#define MAIN_THREAD 0
#define CHILD_THREAD 1

class CallJava {

public:
    _JavaVM *javaVM = NULL;//这个_JavaVM和JavaVM有什么不同？
    JNIEnv *jniEnv = NULL;
    jobject jobj;

    jmethodID jmid_timeinfo;
    jmethodID jmid_error;
    jmethodID jmid_complete;
    jmethodID jmid_prepare;
    jmethodID jmid_supportdecode;
    jmethodID jmid_initMediaCodec;
    jmethodID jmid_decodeAVPacket;

    CallJava(_JavaVM *javaVM, JNIEnv *env, jobject *obj);
    ~CallJava();

    void onCallTimeInfo(int type, int currentTime, int totalTime);

    void onCallError(int type, int code, char *msg);

    void onCallComplete(int type);

    void onCallPrepare(int type);

    bool onCallSupportDecode(const char *codecName);
    //初始化MediaCodec，uint8_t *是数组csd_0的数组头指针
    void onCallInitMediaCodec(const char *codecName, int width, int height,
                              int csd0_size, int csd1_size, uint8_t *csd_0, uint8_t *csd_1);
    //回调java层，用MediaCodec解码AVPacket获取的数据
    void onCallDecodeAVPacket(int data_size, uint8_t *data);

};


#endif //SLPLAYER_CALLJAVA_H

//
// Created by dell on 2019/03/08.
//

#include "CallJava.h"

CallJava::CallJava(_JavaVM *javaVM, JNIEnv *env, jobject *obj) {
    this->javaVM = javaVM;
    this->jniEnv = env;
    this->jobj = *obj;
    this->jobj = env->NewGlobalRef(jobj);//创建全局jobj

    jclass jlz = jniEnv->GetObjectClass(jobj);
    if (!jlz) {
        LOGE("can't get jclass")
        return;
    }

    jmid_timeinfo = env->GetMethodID(jlz, "onCallTimeInfo", "(II)V");
    jmid_error = env->GetMethodID(jlz, "onCallError", "(ILjava/lang/String;)V");
    jmid_complete = env->GetMethodID(jlz, "onCallComplete", "()V");
    jmid_prepare = env->GetMethodID(jlz, "onCallPrepare", "()V");
    jmid_supportdecode = env->GetMethodID(jlz, "onCallSupportMediaDecode", "(Ljava/lang/String;)Z");
    jmid_initMediaCodec = env->GetMethodID(jlz, "onCallInitMediaCodec", "(Ljava/lang/String;II[B[B)V");
    jmid_decodeAVPacket = env->GetMethodID(jlz, "onDecodecAVPacket", "(I[B)V");
}

CallJava::~CallJava() {
    LOGE("~CallJava");
}

void CallJava::onCallTimeInfo(int type, int currentTime, int totalTime) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_timeinfo, currentTime, totalTime);
    } else if (type == CHILD_THREAD) {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            LOGE("call onCallTimeInfo Wrong!");
            return;
        }
        jniEnv->CallVoidMethod(jobj, jmid_timeinfo, currentTime, totalTime);
        javaVM->DetachCurrentThread();
    }
}

void CallJava::onCallError(int type, int code, char *msg) {
    if (type == MAIN_THREAD) {
        jstring jmsg = jniEnv->NewStringUTF(msg);
        jniEnv->CallVoidMethod(jobj, jmid_error, code, jmsg);
        jniEnv->DeleteLocalRef(jmsg);
    } else if (type == CHILD_THREAD) {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            LOGE("call onCallError Wrong!");
            return;
        }
        jstring jmsg = jniEnv->NewStringUTF(msg);
        jniEnv->CallVoidMethod(jobj, jmid_error, code, jmsg);
        jniEnv->DeleteLocalRef(jmsg);
        javaVM->DetachCurrentThread();
    }
}

void CallJava::onCallComplete(int type) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_complete);
    } else if (type == CHILD_THREAD) {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            LOGE("call onCallComplete Wrong!");
            return;
        }
        jniEnv->CallVoidMethod(jobj, jmid_complete);
        javaVM->DetachCurrentThread();
    }
}

void CallJava::onCallPrepare(int type) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_prepare);
    } else if (type == CHILD_THREAD) {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            LOGE("call onCallPrepare Wrong!");
            return;
        }
        jniEnv->CallVoidMethod(jobj, jmid_prepare);
        javaVM->DetachCurrentThread();
    }
}

bool CallJava::onCallSupportDecode(const char *codecName) {
    bool support = false;
    JNIEnv *jniEnv;
    if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
        LOGE("call onCallSupportDecode Wrong!");
        return support;
    }

    jstring type = jniEnv->NewStringUTF(codecName);
    support = jniEnv->CallBooleanMethod(jobj, jmid_supportdecode, type);
    jniEnv->DeleteLocalRef(type);
    javaVM->DetachCurrentThread();

    return support;
}

void CallJava::onCallInitMediaCodec(const char *codecName, int width, int height, int csd0_size,
                                    int csd1_size, uint8_t *csd_0, uint8_t *csd_1) {
    JNIEnv *jniEnv;
    if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
        LOGE("call onCallInitMediaCodec Wrong!");
        return;
    }

    jstring type = jniEnv->NewStringUTF(codecName);
    jbyteArray csd0 = jniEnv->NewByteArray(csd0_size);
    jniEnv->SetByteArrayRegion(csd0, 0, csd0_size, (const jbyte *)(csd_0));
    jbyteArray csd1 = jniEnv->NewByteArray(csd1_size);
    jniEnv->SetByteArrayRegion(csd1, 0, csd1_size, (const jbyte *)(csd_1));

    jniEnv->CallVoidMethod(jobj, jmid_initMediaCodec, type, width, height, csd0, csd1);

    jniEnv->DeleteLocalRef(type);
    jniEnv->DeleteLocalRef(csd0);
    jniEnv->DeleteLocalRef(csd1);
    javaVM->DetachCurrentThread();
}

void CallJava::onCallDecodeAVPacket(int data_size, uint8_t *data) {
    JNIEnv *jniEnv;
    if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
        LOGE("call onCallDecodeAVPacket Wrong!");
        return;
    }

    jbyteArray packet_data = jniEnv->NewByteArray(data_size);
    jniEnv->SetByteArrayRegion(packet_data, 0, data_size, (const jbyte *)data);

    jniEnv->CallVoidMethod(jobj, jmid_decodeAVPacket, data_size, packet_data);

    jniEnv->DeleteLocalRef(packet_data);
    javaVM->DetachCurrentThread();
}

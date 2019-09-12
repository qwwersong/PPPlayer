#include <jni.h>
#include <string>
#include "ffmpeg/FFmpegManager.h"

_JavaVM *javaVM = NULL;
CallJava *callJava = NULL;
jobject java_surface = NULL;

FFmpegManager *ffmpegManager;

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved){
    jint result = -1;
    javaVM = vm;
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return result;
    }
    return JNI_VERSION_1_4;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_songlei_slplayer_player_Player_setSurface(JNIEnv *env, jobject instance, jobject surface) {

    //这个=号并不是java中的赋值拷贝，在C中赋值拷贝是不一样的
    java_surface = env->NewGlobalRef(surface);//创建全局jobj
    if (ffmpegManager != NULL && java_surface != NULL) {
        ffmpegManager->baseSurface = new BaseSurface(env, java_surface);
        ffmpegManager->setSurface(ffmpegManager->baseSurface);
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_songlei_slplayer_player_Player_onSurfaceChange(JNIEnv *env, jobject instance, jint width, jint height) {

    if (ffmpegManager != NULL) {
        ffmpegManager->baseSurface->onSurfaceChange(width, height);
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_songlei_slplayer_player_Player_init(JNIEnv *env, jobject instance) {

    if (ffmpegManager == NULL) {
        callJava = new CallJava(javaVM, env, &instance);
        ffmpegManager = new FFmpegManager(callJava);
        ffmpegManager->initFFmpeg();
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_songlei_slplayer_player_Player_prepare(JNIEnv *env, jobject instance, jstring url_) {
    const char *url = env->GetStringUTFChars(url_, 0);

    if (ffmpegManager != NULL) {
        ffmpegManager->connect(url);
    }

    env->ReleaseStringUTFChars(url_, url);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_songlei_slplayer_player_Player_start(JNIEnv *env, jobject instance) {
//    const char *url = env->GetStringUTFChars(url_, 0);

    if (ffmpegManager != NULL) {
//        ffmpegManager->connect(url);
        ffmpegManager->start();
    }

//    env->ReleaseStringUTFChars(url_, url);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_songlei_slplayer_player_Player_pause(JNIEnv *env, jobject instance) {

    if (ffmpegManager != NULL) {
        ffmpegManager->pause();
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_songlei_slplayer_player_Player_resume(JNIEnv *env, jobject instance) {

    if (ffmpegManager != NULL) {
        ffmpegManager->resume();
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_songlei_slplayer_player_Player_stop(JNIEnv *env, jobject instance) {

    if (ffmpegManager != NULL) {
        ffmpegManager->release();
        delete(ffmpegManager);
        ffmpegManager = NULL;
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_songlei_slplayer_player_Player_seek(JNIEnv *env, jobject instance, jint sec) {

    if (ffmpegManager != NULL) {
        ffmpegManager->seek(sec);
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_songlei_slplayer_player_Player_seekVolume(JNIEnv *env, jobject instance, jint volume) {

    if (ffmpegManager != NULL) {
        ffmpegManager->setVolume(volume);
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_songlei_slplayer_player_Player_setChannelMute(JNIEnv *env, jobject instance, jint type) {

    if (ffmpegManager != NULL) {
        ffmpegManager->setChannelMute(type);
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_songlei_slplayer_player_Player_setPitch(JNIEnv *env, jobject instance, jfloat pitch) {

    if (ffmpegManager != NULL) {
        ffmpegManager->setPitch(pitch);
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_songlei_slplayer_player_Player_setSpeed(JNIEnv *env, jobject instance, jfloat speed) {

    if (ffmpegManager != NULL) {
        ffmpegManager->setSpeed(speed);
    }

}
//
// Created by dell on 2019/02/26.
//

#ifndef SLPLAYER_BASESURFACE_H
#define SLPLAYER_BASESURFACE_H

#include <jni.h>
#include "ShaderUtil.h"
#include "android/native_window.h"
#include "android/native_window_jni.h"
#include "EGLThread.h"
#include "MatrixUtil.h"
#include "../utils/AndroidLog.h"

extern "C"
{
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
};

class BaseSurface {

public:
    ANativeWindow *nativeWindow = NULL;
    EGLThread *eglThread = NULL;
    AVCodecContext *avCodecContext = NULL;
    AVFrame *avFrame = NULL;

    GLuint  vboId;

public:
    BaseSurface(JNIEnv *env, jobject surface);
    ~BaseSurface();

    void drawFrame(AVCodecContext *avCodecContext, AVFrame *avFrame);

    void onSurfaceChange(int width, int height);

};


#endif //SLPLAYER_BASESURFACE_H

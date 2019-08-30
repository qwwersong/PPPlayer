//
// Created by dell on 2019/02/18.
//

#ifndef TIKTOKDEMO_EGLHELPER_H
#define TIKTOKDEMO_EGLHELPER_H

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <jni.h>
#include "../utils/AndroidLog.h"

class EGLHelper {

public:
    ANativeWindow *window = NULL;
    EGLDisplay eglDisplay = NULL;
    EGLSurface eglSurface = NULL;
    EGLContext eglContext = NULL;

public:
    EGLHelper();
    ~EGLHelper();

    void init(ANativeWindow *win);

    void swapBuffers();

    void destroyEgl();

    EGLContext getEglContext();
};


#endif //TIKTOKDEMO_EGLHELPER_H

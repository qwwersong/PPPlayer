//
// Created by dell on 2019/02/28.
//

#ifndef SLPLAYER_EGLTHREAD_H
#define SLPLAYER_EGLTHREAD_H

#include "pthread.h"
#include "EGLHelper.h"
#include "android/native_window.h"
#include <unistd.h>

#define OPENGL_RENDER_AUTO 1
#define OPENGL_RENDER_HANDLE 2

class EGLThread {

public:
    pthread_t eglThread = -1;
    ANativeWindow *nativeWindow = NULL;
    EGLHelper *eglHelper = NULL;

    bool isCreate = false;
    bool isChange = false;
    bool isExit = false;

    //回调
    typedef void (*OnCreate)(void *);   //回调方法
    OnCreate onCreate;                    //回调变量
    void *onCreateCtx;                    //回调方法的参数

    typedef void (*OnChange)(int width, int height, void *);
    OnChange onChange;
    void *onChangeCtx;

    typedef void (*OnDraw)(void *);
    OnDraw onDraw;
    void *onDrawCtx;

    int renderType = OPENGL_RENDER_AUTO;

    pthread_mutex_t pthread_mutex;
    pthread_cond_t pthread_cond;

    int surfaceWidth = 0;
    int surfaceHeight = 0;


public:
    EGLThread();
    ~EGLThread();

    void onSurfaceCreate(ANativeWindow *window);
    void onSurfaceChange(int width, int height);

    void setOnCreateCallBack(OnCreate onCreate, void *ctx);
    void setOnChangeCallBack(OnChange onChange, void *ctx);
    void setOnDrawCallBack(OnDraw onDraw, void *ctx);

    void setRenderType(int renderType);
    void notifyRender();

};


#endif //SLPLAYER_EGLTHREAD_H

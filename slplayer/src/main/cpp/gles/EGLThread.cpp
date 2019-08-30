//
// Created by dell on 2019/02/28.
//

#include "EGLThread.h"

EGLThread::EGLThread() {
    eglHelper = new EGLHelper();
    pthread_mutex_init(&pthread_mutex, NULL);
    pthread_cond_init(&pthread_cond, NULL);
}

EGLThread::~EGLThread() {
    LOGE("~EGLThread()")
    eglHelper->destroyEgl();
    pthread_mutex_destroy(&pthread_mutex);
    pthread_cond_destroy(&pthread_cond);
}

void *eglCallBack(void *ctx){
    EGLThread *eglThread = static_cast<EGLThread *>(ctx);
    eglThread->eglHelper->init(eglThread->nativeWindow);
    eglThread->isExit = false;

    while (true) {
        if (eglThread->isCreate) {     //onCreate回调
            eglThread->isCreate = false;
            eglThread->onCreate(eglThread->onCreateCtx);
        }
        if (eglThread->isChange) {     //onChange回调
            eglThread->isChange = false;
            eglThread->onChange(eglThread->surfaceWidth, eglThread->surfaceHeight, eglThread->onChangeCtx);
        }
        //onDraw回调
        eglThread->onDraw(eglThread->onDrawCtx);
        eglThread->eglHelper->swapBuffers();

        //设置renderMode
        if (eglThread->renderType == OPENGL_RENDER_AUTO) {
            usleep(1000000 / 60);
        } else if (eglThread->renderType == OPENGL_RENDER_HANDLE) {
            pthread_mutex_lock(&eglThread->pthread_mutex);
            pthread_cond_wait(&eglThread->pthread_cond, &eglThread->pthread_mutex);
            pthread_mutex_unlock(&eglThread->pthread_mutex);
        } else {
            LOGE("unknow render type");
        }
        //退出
        if (eglThread->isExit) {
            break;
        }
    }

    pthread_exit(&eglThread->eglThread);
}

void EGLThread::onSurfaceCreate(ANativeWindow *window) {
    if (eglThread == -1) {
        nativeWindow = window;
        isCreate = true;
        pthread_create(&eglThread, NULL, eglCallBack, this);
    }
}

void EGLThread::onSurfaceChange(int width, int height) {
    isChange = true;
    surfaceWidth = width;
    surfaceHeight = height;
    notifyRender();
}

void EGLThread::setOnCreateCallBack(EGLThread::OnCreate onCreate, void *ctx) {
    this->onCreate = onCreate;
    this->onCreateCtx = ctx;
}

void EGLThread::setOnChangeCallBack(EGLThread::OnChange onChange, void *ctx) {
    this->onChange = onChange;
    this->onChangeCtx = ctx;
}

void EGLThread::setOnDrawCallBack(EGLThread::OnDraw onDraw, void *ctx) {
    this->onDraw = onDraw;
    this->onDrawCtx = ctx;
}

void EGLThread::setRenderType(int renderType) {
    this->renderType = renderType;
}

void EGLThread::notifyRender() {
    pthread_mutex_lock(&pthread_mutex);
    pthread_cond_signal(&pthread_cond);
    pthread_mutex_unlock(&pthread_mutex);
}
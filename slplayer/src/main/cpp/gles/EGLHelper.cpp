//
// Created by dell on 2019/02/18.
//

#include "EGLHelper.h"

EGLHelper::EGLHelper() {

}

EGLHelper::~EGLHelper() {

}

void EGLHelper::init(ANativeWindow *win) {
    window = win;

    //1、创建eglDisplay
    eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);//创建EGL显示设备，选择默认
    if (eglDisplay == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay failed");
        return;
    }

    //==================================2、EGL初始化========================================
    /*
         EGLAPI EGLBoolean EGLAPIENTRY eglInitialize ( // 初始化EGLdisplay
             EGLDisplay dpy, // display
             EGLint *major, // 主版本号
             EGLint *minor // 次版本号， 两个都传空表示使用默认值 2.0 ?
         );
     */
    if (eglInitialize(eglDisplay, 0, 0) != EGL_TRUE) {
        LOGE("eglInitialize failed");
        return;
    }

    EGLConfig eglConfig;//输出的config
    EGLint configNum;//输出的config数量
    EGLint configSpec[] = {//输出的
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 5,
            EGL_GREEN_SIZE, 6,
            EGL_RED_SIZE, 5,
            EGL_NONE
    };
//            EGL_RED_SIZE, 5,//表示r占8位
//            EGL_GREEN_SIZE, 6,//表示g占8位
//            EGL_BLUE_SIZE, 5,//表示b占8位
//            EGL_SURFACE_TYPE, EGL_WINDOW_BIT, // EGL_SURFACE_TYPE 类型对应的是 EGL_WINDOW_BIT
//            EGL_NONE //最后的 EGL_NONE 表示数组的结尾处NULL，用于数组遍历结束判断

    /*
        EGLAPI EGLBoolean EGLAPIENTRY eglChooseConfig ( // 设置egl surface配置项
            EGLDisplay dpy, // display
            const EGLint *attrib_list, // 输入的参数列表
            EGLConfig *configs, // 输出的配置项
            EGLint config_size, // 最多存储多少个输出的配置项
            EGLint *num_config // 实际存储的输出配置项的个数， <= config_size
        );
    */

    // 设置egl surface配置项
    if (!eglChooseConfig(eglDisplay, configSpec, &eglConfig, 1, &configNum)) {
        LOGE("eglChooseConfig failed");
        return;
    }

    /*
         EGLAPI EGLSurface EGLAPIENTRY eglCreateWindowSurface ( // 创建 egl surface
             EGLDisplay dpy, // display
             EGLConfig config, // egl surface配置项
             EGLNativeWindowType win, // 本地窗口， EGLNativeWindowType 就是 ANativeWindow *
             const EGLint *attrib_list // 属性信息，用来设置版本号，传空表示默认版本号
         );
     */
    // 创建 egl surface后端显示，绘制数据的缓冲区域
    eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, window, NULL);
    if (eglSurface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed");
        return;
    }

    //==================================3、创建EGLContext========================================
    //通过EGL关联窗口与OpenGL的上下文
    const EGLint ctxAttr[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE //EGL_CONTEXT_CLIENT_VERSION,2  表示EGL版本号为2
    };

    /*
         EGLAPI EGLContext EGLAPIENTRY eglCreateContext ( // 创建关联系统窗口与OpenGL的上下文
             EGLDisplay dpy, // display
             EGLConfig config, // 配置信息
             EGLContext share_context, // 多个显示设备共享EGLContext的时候使用，这里用不到共享，所以传 EGL_NO_CONTEXT
             const EGLint *attrib_list // EGL版本信息
         );
     */
    eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, ctxAttr);
    if (eglContext == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed");
        return;
    }

    /*
         EGLAPI EGLBoolean EGLAPIENTRY eglMakeCurrent ( // 数据交互
             EGLDisplay dpy, // display
             EGLSurface draw, // 用来绘制的 EGLSurface
             EGLSurface read, // 用来读取的 EGLSurface
             EGLContext ctx // 上下文
         );
     */
    if (eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext) != EGL_TRUE) {
        LOGE("eglMakeCurrent failed");
        return;
    }
    LOGE("egl init success");
}

void EGLHelper::swapBuffers() {
    if (eglDisplay != NULL && eglSurface != NULL) {
        //将数据先绘制到eglSurface的缓冲区中，然后利用eglSwapBuffer将eglSurface数据冲到eglDisplay中取显示
        eglSwapBuffers(eglDisplay, eglSurface);
    } else {
        LOGE("eglDisplay or eglSurface is NULL");
    }

}

void EGLHelper::destroyEgl() {
    LOGE("destroyEgl");
    eglDestroyContext(eglDisplay, eglContext);
    eglDestroySurface(eglDisplay, eglSurface);
    ANativeWindow_release(window);
    eglDisplay = NULL;
    eglContext = NULL;
    eglSurface = NULL;
    window = NULL;
}

EGLContext EGLHelper::getEglContext() {
    return eglContext;
}

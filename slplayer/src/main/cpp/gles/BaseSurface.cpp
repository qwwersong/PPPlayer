//
// Created by dell on 2019/02/26.
//

//#include <android/log.h>
#include "BaseSurface.h"

#define GET_STR(x) #x
const char *vertexShaderString = GET_STR(
        uniform mat4 uMatrix;
        attribute vec4 aPosition;
        attribute vec2 aTexCoord;
        varying vec2 vTexCoord;
        void main() {
            vTexCoord = aTexCoord;
            gl_Position = uMatrix * aPosition;
        }
);
const char *fragmentShaderString = GET_STR(
        precision mediump float;
        varying vec2 vTexCoord;
        uniform sampler2D yTexture;
        uniform sampler2D uTexture;
        uniform sampler2D vTexture;
        void main() {
            vec3 yuv;
            yuv.x = texture2D(yTexture, vTexCoord).r;
            yuv.y = texture2D(uTexture, vTexCoord).r - 0.5;
            yuv.z = texture2D(vTexture, vTexCoord).r - 0.5;

            vec3 rgb;
            rgb = mat3(
                    1,       1,        1,
                    0,       -0.39465, 2.03211,
                    1.13983, -0.58060, 0
            ) * yuv;

            gl_FragColor = vec4(rgb, 1);
        }

//        const mediump mat3 yuv2rgb = mat3(
//        1.0,	1.0,	1.0,
//        0,		-0.391, 2.018,
//        1.596,	-0.813, 0
//);
//        const mediump vec3 lumCoeff = vec3(0.2125,0.7154,0.0721);
//
//        void main()
//        {
//            mediump vec3 yuv = vec3(
//                    1.1643 * (texture2D(yTexture, vTexCoord).r - 0.0627),
//                    (texture2D(uTexture, vTexCoord).r - 0.502),
//                    (texture2D(vTexture, vTexCoord).r - 0.502)
//            );
//
//
//            mediump vec3 rgb = yuv2rgb*yuv;
//            mediump vec3 intensity = vec3(dot(rgb,lumCoeff));
//            mediump vec3 color = mix(intensity,rgb.rgb,1.1);
//            color = clamp(color,0.0,255.0);
//
//            gl_FragColor = vec4(color, 1.0);
//        }
);

float vertexData[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f, 1.0f,
        1.0f, 1.0f
};

float textureData[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
};

float *projectionMatrix = new float[16];

GLuint yTextureId;
GLuint uTextureId;
GLuint vTextureId;

GLuint textureSamplerHandleY;
GLuint textureSamplerHandleU;
GLuint textureSamplerHandleV;

GLuint programId;

GLuint aPositionHandle;
GLuint aTextureCoordHandle;
GLuint uMatrixLocation;

int viewWidth;
int viewHeight;

void SurfaceCreate_CallBack(void *data){
    LOGE("SurfaceCreate_CallBack");
    BaseSurface *baseSurface = static_cast<BaseSurface *>(data);
    programId = createProgram(vertexShaderString, fragmentShaderString);
    aPositionHandle = (GLuint) glGetAttribLocation(programId, "aPosition");
    aTextureCoordHandle = (GLuint) glGetAttribLocation(programId, "aTexCoord");

    textureSamplerHandleY = (GLuint) glGetUniformLocation(programId, "yTexture");
    textureSamplerHandleU = (GLuint) glGetUniformLocation(programId, "uTexture");
    textureSamplerHandleV = (GLuint) glGetUniformLocation(programId, "vTexture");
    uMatrixLocation = (GLuint) glGetUniformLocation(programId, "uMatrix");

    /***
     * 初始化空的yuv纹理
     * **/
    glGenTextures(1, &yTextureId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, yTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //TODO::数组指针貌似不能获取数组长度的功能
    glGenBuffers(1, &baseSurface->vboId);
    LOGE("sizeof(vertex_array) = %d", sizeof(vertexData));
    //绑定VBO
    glBindBuffer(GL_ARRAY_BUFFER, baseSurface->vboId);
    //分配VBO缓存空间
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData) + sizeof(textureData), NULL, GL_STATIC_DRAW);
    //设置VBO顶点数据值
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertexData), vertexData);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertexData), sizeof(textureData), textureData);
    //解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenTextures(1, &uTextureId);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, uTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    glGenTextures(1, &vTextureId);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, vTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void SurfaceChange_CallBack(int width, int height, void *data){
    LOGE("SurfaceChange_CallBack");
    LOGE("SurfaceChange_CallBack w = %d", width);
    LOGE("SurfaceChange_CallBack h = %d", height);
    BaseSurface *baseSurface = static_cast<BaseSurface *>(data);
    viewWidth = width;
    viewHeight = height;
    glViewport(0, 0, width, height);
}

void SurfaceDraw_CallBack(void *data){
    BaseSurface *baseSurface = static_cast<BaseSurface *>(data);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);       //这两个顺序不能错
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);//这两个顺序不能错

    if (baseSurface->avCodecContext != NULL && baseSurface->avFrame != NULL &&
            baseSurface->avCodecContext->width > 0 && baseSurface->avCodecContext->height > 0 &&
            baseSurface->avFrame->data[0] != NULL && baseSurface->avFrame->data[1] != NULL &&
            baseSurface->avFrame->data[2] != NULL) {

        int videoWidth = baseSurface->avCodecContext->width;
        int videoHeight = baseSurface->avCodecContext->height;

//        LOGE("onDraw videoWidth = %d", videoWidth);
//        LOGE("onDraw videoHeight = %d", videoHeight);
        float videoAspectRatio = (float)videoWidth / (float)videoHeight;
        if (viewWidth > viewHeight) {//横屏
            if (videoWidth > videoHeight) {//视频源是横屏
                orthoM(projectionMatrix, 0, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
            } else {//视频源是竖屏
                orthoM(projectionMatrix, 0, -videoAspectRatio, videoAspectRatio, -1.0f, 1.0f, -1.0f, 1.0f);
            }
        } else {//竖屏
            if (videoWidth > videoHeight) {//视频源是横屏
                orthoM(projectionMatrix, 0, -1.0f, 1.0f, -videoAspectRatio, videoAspectRatio, -1.0f, 1.0f);
            } else {//视频源是竖屏
                LOGE("在这呢");
                orthoM(projectionMatrix, 0, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
            }
        }

        glUseProgram(programId);
//        glBindBuffer(GL_ARRAY_BUFFER, baseSurface->vboId);//VBO写在glVertexAttribPointer之前会黑屏

        glEnableVertexAttribArray(aPositionHandle);
        glVertexAttribPointer(aPositionHandle, 2, GL_FLOAT, GL_FALSE, 8, vertexData);
        glEnableVertexAttribArray(aTextureCoordHandle);
        glVertexAttribPointer(aTextureCoordHandle, 2, GL_FLOAT, GL_FALSE, 8, textureData);

        glUniformMatrix4fv(uMatrixLocation, 1, false, projectionMatrix);

//        LOGE("Y = %d", baseSurface->avFrame->linesize[0])
//        LOGE("U = %d", baseSurface->avFrame->linesize[1])
//        LOGE("V = %d", baseSurface->avFrame->linesize[2])
//        int width = baseSurface->avFrame->linesize[0];

        //avFrame->linesize 是640，avFrame->data 里包含绿边的数据
        //avFrame->width 是630，那10个像素就是绿边的宽度。avFrame->data是一个指针指向的就是一帧数据的头地址。
        //两种方法设置对齐，
        // 一：拷贝630的数据到另一个内存区域中。
        // 二：利用openGL对顶点坐标做处理。不能用矩阵缩放等操作，因为绿边在data数据中，矩阵操作是对整个顶点坐标的，绿边数据还是会映射到
        glBindBuffer(GL_ARRAY_BUFFER, baseSurface->vboId);//VBO写在glVertexAttribPointer之后好使

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, yTextureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, baseSurface->avFrame->linesize[0], videoHeight, 0,
                     GL_LUMINANCE, GL_UNSIGNED_BYTE, baseSurface->avFrame->data[0]);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, uTextureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, baseSurface->avFrame->linesize[1], videoHeight / 2, 0,
                     GL_LUMINANCE, GL_UNSIGNED_BYTE, baseSurface->avFrame->data[1]);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, vTextureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, baseSurface->avFrame->linesize[2], videoHeight / 2, 0,
                     GL_LUMINANCE, GL_UNSIGNED_BYTE, baseSurface->avFrame->data[2]);

        glUniform1i(textureSamplerHandleY, 0);
        glUniform1i(textureSamplerHandleU, 1);
        glUniform1i(textureSamplerHandleV, 2);

//        FILE* file = fopen("YUV420P.YUV", "ab+");
//        fwrite(baseSurface->avFrame->data[0], 1, static_cast<size_t>(baseSurface->avFrame->linesize[0] * baseSurface->avFrame->height), file);
//        fwrite(baseSurface->avFrame->data[1], 1, static_cast<size_t>(baseSurface->avFrame->linesize[1] * baseSurface->avFrame->height / 2), file);
//        fwrite(baseSurface->avFrame->data[2], 1, static_cast<size_t>(baseSurface->avFrame->linesize[2] * baseSurface->avFrame->height / 2), file);
//        fclose(file);
    }
    //放在外面对应闪屏，因为有些帧没进入条件内
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    //必须要释放，不释放的话占用内存越来越高，会越来越卡
    av_frame_free(&baseSurface->avFrame);
    av_free(baseSurface->avFrame);
    baseSurface->avFrame = NULL;
}

BaseSurface::BaseSurface(JNIEnv *env, jobject surface) {
    nativeWindow = ANativeWindow_fromSurface(env, surface);
    eglThread = new EGLThread();
    eglThread->setRenderType(OPENGL_RENDER_HANDLE);
    eglThread->setOnCreateCallBack(SurfaceCreate_CallBack, this);
    eglThread->setOnChangeCallBack(SurfaceChange_CallBack, this);
    eglThread->setOnDrawCallBack(SurfaceDraw_CallBack, this);

    eglThread->onSurfaceCreate(nativeWindow);
}

BaseSurface::~BaseSurface() {
//释放内存
    LOGE("~BaseSurface()");
    if (eglThread != NULL) {
        LOGE("~BaseSurface() notifyRender");
        eglThread->isExit = true;
        eglThread->notifyRender();
        delete(eglThread);
        eglThread = NULL;
    }
    if (avCodecContext != NULL) {
        avcodec_close(avCodecContext);
        avcodec_free_context(&avCodecContext);
        avCodecContext = NULL;
    }
    if (nativeWindow != NULL) {
        nativeWindow = NULL;
    }
}

void BaseSurface::drawFrame(AVCodecContext *avCodecContext, AVFrame *avFrame) {
    this->avCodecContext = avCodecContext;
    this->avFrame = avFrame;
    eglThread->notifyRender();
}

void BaseSurface::onSurfaceChange(int width, int height) {
    eglThread->onSurfaceChange(width, height);
}

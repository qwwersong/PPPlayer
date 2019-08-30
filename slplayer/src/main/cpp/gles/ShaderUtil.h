//
// Created by dell on 2019/02/20.
//

#ifndef TIKTOKDEMO_SHADERUTIL_H
#define TIKTOKDEMO_SHADERUTIL_H

#include <stdlib.h> //malloc定义头文件
#include <GLES2/gl2.h>
#include "../utils/AndroidLog.h"

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error = glGetError()) {
        LOGE("after %s() glError (0x%x)\n", op, error);
    }
}

static GLuint loadShader(GLenum type, const char *shaderSrc) {
    GLuint shader;
    GLint compiled;

    //创建shader
    shader = glCreateShader(type);
    if (shader == 0) {
        return 0;
    }
    //加载shader的源码
    glShaderSource(shader, 1, &shaderSrc, NULL);
    //编译源码
    glCompileShader(shader);
    //检查编译状态
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        //编译未通过
        GLint infoLen = 0;
        //查询日志的长度判断是否有日志产生
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1) {
            //分配一个足以存储日志信息的字符串
            char *infoLog = (char *) malloc(sizeof(char) * infoLen);
            //检索日志信息
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            //使用完成后需要释放字符串所分配的内存
            free(infoLog);
        }
        //删除编译出错的着色器，释放内存
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static GLuint createProgram(const char *vertexShaderStr, const char *fragShaderStr) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexShaderStr);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, fragShaderStr);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

#endif //TIKTOKDEMO_SHADERUTIL_H

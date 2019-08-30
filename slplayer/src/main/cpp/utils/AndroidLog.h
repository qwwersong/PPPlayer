//
// Created by dell on 2019/01/18.
//
#pragma once //只引用一次
#ifndef TIKTOKDEMO_ANDROIDLOG_H
#define TIKTOKDEMO_ANDROIDLOG_H

#include <android/log.h>
#define LOGD(FORMAT,...) __android_log_print(ANDROID_LOG_DEBUG,"xx",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"xx",FORMAT,##__VA_ARGS__);

#define LOG_DEBUG false

#endif //TIKTOKDEMO_ANDROIDLOG_H

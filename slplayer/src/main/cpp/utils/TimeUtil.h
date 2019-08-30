//
// Created by dell on 2019/03/04.
//

#ifndef SLPLAYER_TIMEUTIL_H
#define SLPLAYER_TIMEUTIL_H

#include <chrono>

#define __DEBUG__TIME__ON
#define  millisecond 1000000
//run times test...
#ifdef  __DEBUG__TIME__ON
#define LOG_TIME  LOGE
#define RUN_TIME(time)  (double)(time).count()/millisecond
//#define RUN_TIME(...)  getTime_MS( __VA_ARGS__)

#define DEBUG__TIME0  auto TIME0= std::chrono::system_clock::now()
#define DEBUG__TIME1  auto TIME1= std::chrono::system_clock::now()

#else
#define DEBUG__TIME0
#define DEBUG__TIME1
#define LOG_TIME(...)
#endif

#endif //SLPLAYER_TIMEUTIL_H

//
// Created by dell on 2019/03/04.
//

#ifndef SLPLAYER_MATRIXUTIL_H
#define SLPLAYER_MATRIXUTIL_H

#include "../utils/AndroidLog.h"

//矩阵缩放
static void orthoM(float * m, int mOffset,
                   float left, float right, float bottom, float top,
                   float near, float far){
    if (left == right) {
        LOGE("left == right");
        return;
    }
    if (bottom == top) {
        LOGE("bottom == top");
        return;
    }
    if (near == far) {
        LOGE("near == far");
        return;
    }

    float r_width  = 1.0f / (right - left);
    float r_height = 1.0f / (top - bottom);
    float r_depth  = 1.0f / (far - near);
    float x =  2.0f * (r_width);
    float y =  2.0f * (r_height);
    float z = -2.0f * (r_depth);
    float tx = -(right + left) * r_width;
    float ty = -(top + bottom) * r_height;
    float tz = -(far + near) * r_depth;
    m[mOffset + 0] = x;
    m[mOffset + 5] = y;
    m[mOffset +10] = z;
    m[mOffset +12] = tx;
    m[mOffset +13] = ty;
    m[mOffset +14] = tz;
    m[mOffset +15] = 1.0f;
    m[mOffset + 1] = 0.0f;
    m[mOffset + 2] = 0.0f;
    m[mOffset + 3] = 0.0f;
    m[mOffset + 4] = 0.0f;
    m[mOffset + 6] = 0.0f;
    m[mOffset + 7] = 0.0f;
    m[mOffset + 8] = 0.0f;
    m[mOffset + 9] = 0.0f;
    m[mOffset + 11] = 0.0f;
}

#endif //SLPLAYER_MATRIXUTIL_H

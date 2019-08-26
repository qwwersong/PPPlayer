package com.songlei.obssplayer;

/**
 * Created by songlei on 2018/11/12.
 */
public interface OnObssListener {

    /**
     * 播放异常
     */
    void onError(PlayException e);

    /**
     * 当前播放状态
     * @param code 播放状态码
     */
    void onPlayState(int code);

    /**
     * 缓冲更新
     * @param percent 缓冲进度
     */
    void onBufferingUpdate(int percent);

}

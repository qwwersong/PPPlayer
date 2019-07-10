package com.songlei.xplayer.obssplayer;

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

}

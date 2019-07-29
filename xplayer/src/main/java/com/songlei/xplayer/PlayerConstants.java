package com.songlei.xplayer;

/**
 * 播放器常量
 * Created by songlei on 2019/07/10.
 */
public class PlayerConstants {
    //========================播放器状态========================
    //未播放
    public static final int STATE_NO_PLAY = 0;
    //准备
    public static final int STATE_PREPARE = 1;
    //播放
    public static final int STATE_PLAYING = 2;
    //暂停
    public static final int STATE_PAUSE = 3;
    //结束
    public static final int STATE_COMPLETE = 4;
    //异常
    public static final int STATE_ERROR = 5;
    //缓冲
    public static final int STATE_BUFFERING = 6;

}

package com.songlei.xplayer.obssplayer;

/**
 * Created by songlei on 2018/11/19.
 */
public class PlayException extends Exception {
    public static final int VPC_OUT_OF_MEMORY = 1;      //内存不足
    public static final int VPC_NO_SOURCE_DEMUX = 2;    //没有数据处理器
    public static final int VPC_NETWORK_ERROR = 22;     //网络错误
    public static final int VPC_MEDIA_SPEC_ERROR = 23;  //数据格式错误
    public static final int VPC_NO_PLAY_OBJECT = 24;    //无播放对象
    public static final int VPC_NET_TIME_OUT = 25;      //网络超时

    public static final int MEDIA_PLAYER_ERROR = 1001;  //mediaplayer错误码

    public int code;           //错误码
    public String cause;       //错误信息

    public PlayException(int code, String cause){
        this.code = code;
        this.cause = cause;
    }
}

package com.songlei.xplayer.obssplayer;

/**
 * Created by songlei on 2018/11/21.
 */
public class PlayerConstants {

    public static final int PLAYER_MW = 1;          //梦网自研播放器
    public static final int PLAYER_MEDIA = 2;       //系统播放器

    public enum PlayerType {
        Player_TYPE_MONTNETS, Player_TYPE_MEDIA
    }
    //-----------------播放状态------------------------//
    public static final int STATE_PLAYING = 1;      //正在播放
    public static final int STATE_PAUSE = 2;        //暂停
    public static final int STATE_FINISH = 3;       //结束
    public static final int STATE_BUFFERING = 4;    //缓冲
    public static final int STATE_ERROR = 5;        //错误

    //-----------------直播流状态------------------------//
    public static final int LIVE_STATUS_PREVIEW = 0;        //直播预告或视频未审核
    public static final int LIVE_STATUS_LIVING = 1;         //直播中或视频上线
    public static final int LIVE_STATUS_END = 2;            //直播结束或视频下线
    public static final int LIVE_STATUS_ERROR = 3;          //直播异常中断或视频审核不通过
    public static final int LIVE_STATUS_TIMEOUT = 4;        //直播过期

    //-----------------播放器缩放状态------------------------//
    public static final int VIDEO_SCALE_ORIGIN = 0;         //原始
    public static final int VIDEO_SCALE_FIT = 1;            //适应
    public static final int VIDEO_SCALE_FILL = 2;           //填充

    //-----------------播放器状态------------------------//
    public static final int VPC_OK = 0;                 //
    public static final int VPC_OUT_OF_MEMORY = 1;      //内存不足
    public static final int VPC_NO_SOURCE_DEMUX = 2;    //没有数据处理器

    public static final int VPC_CONNECT_SERVER = 21;    //连接服务器

    public static final int VPC_NETWORK_ERROR = 22;     //网络错误
    public static final int VPC_MEDIA_SPEC_ERROR = 23;  //数据格式错误
    public static final int VPC_NO_PLAY_OBJECT = 24;    //无播放对象
    public static final int VPC_NET_TIME_OUT = 25;      //网络超时
    public static final int VPC_RECV_PENDING = 26;
    public static final int VPC_DELETE_FRAME = 27;

    public static final int VPC_NOTIFY_MEDIA_INFO = 51; //媒体信息通知消息
    public static final int VPC_START_BUFFER_DATA = 52; //开始缓冲数据
    public static final int VPC_PRE_PLAY = 53;          //即将开始播放
    public static final int VPC_START_PLAY = 54;        //开始播放
    public static final int VPC_PLAY_FINISH = 55;       //播放完成
    public static final int VPC_PLAY_BUFFER = 56;       //播放过程中缓冲数据
    public static final int VPC_PLAY_FINISHED = 100;    //已经播放结束
}

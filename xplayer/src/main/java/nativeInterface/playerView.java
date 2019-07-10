package nativeInterface;

import android.content.Context;
import android.net.ConnectivityManager;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;

import com.songlei.xplayer.base.CommonHandler;
import com.songlei.xplayer.obssplayer.BasePlayer;
import com.songlei.xplayer.obssplayer.OnPlayerListener;
import com.songlei.xplayer.obssplayer.PlayException;
import com.songlei.xplayer.obssplayer.PlayerConstants;


/**
 * Created by songlei on 2018/11/08.
 */
public class playerView extends BasePlayer implements CommonHandler.HandlerCallBack {
    private static final String TAG = "playerView";
    private CommonHandler mHandler;
    private OnPlayerListener onPlayerListener;
    private Surface mSurface = null;
    // 默认全屏
    private int mSacleType = PlayerConstants.VIDEO_SCALE_FILL;
    private int playTime;
    private int playState;
    private boolean isStartPlay = false;
    private boolean hasCallNativeOnCreate = false;
    public int mInstance;

    private String url;

    static {
        System.loadLibrary("FraunhoferAAC");
        System.loadLibrary("rtmp");
        System.loadLibrary("FFMPEG");
        System.loadLibrary("playercorejni");
    }

    public playerView(Context context) {
        mHandler = new CommonHandler(this);
        onCreateMWPlayer(context);
    }

    @Override
    public void handleMessage(Message msg) {
        if (onPlayerListener == null) {
            return;
        }
        switch (msg.what) {
            case PlayerConstants.VPC_OUT_OF_MEMORY:
                playState = PlayerConstants.STATE_ERROR;
                onPlayerListener.onError(new PlayException(PlayException.VPC_OUT_OF_MEMORY, "内存不足"));
                break;
            case PlayerConstants.VPC_NO_SOURCE_DEMUX:
                playState = PlayerConstants.STATE_ERROR;
                onPlayerListener.onError(new PlayException(PlayException.VPC_NO_SOURCE_DEMUX, "没有数据处理器"));
                break;
            case PlayerConstants.VPC_MEDIA_SPEC_ERROR:
                playState = PlayerConstants.STATE_ERROR;
                onPlayerListener.onError(new PlayException(PlayException.VPC_MEDIA_SPEC_ERROR, "数据格式错误"));
                break;
            case PlayerConstants.VPC_NO_PLAY_OBJECT:
                playState = PlayerConstants.STATE_ERROR;
                onPlayerListener.onError(new PlayException(PlayException.VPC_NO_PLAY_OBJECT, "无播放对象"));
                break;
            case PlayerConstants.VPC_NET_TIME_OUT:
                playState = PlayerConstants.STATE_ERROR;
                onPlayerListener.onError(new PlayException(PlayException.VPC_NET_TIME_OUT, "网络超时"));
                break;
            case PlayerConstants.VPC_NETWORK_ERROR:
                playState = PlayerConstants.STATE_ERROR;
                onPlayerListener.onError(new PlayException(PlayException.VPC_NETWORK_ERROR, "网络错误"));
                break;
            case PlayerConstants.VPC_START_PLAY:
                playState = PlayerConstants.STATE_PLAYING;
                onPlayerListener.onPlayState(PlayerConstants.STATE_PLAYING);
                break;
            case PlayerConstants.VPC_PLAY_FINISH:
                stop(true);
                break;
            case PlayerConstants.VPC_START_BUFFER_DATA:
            case PlayerConstants.VPC_PLAY_BUFFER:
                playState = PlayerConstants.STATE_BUFFERING;
                onPlayerListener.onPlayState(PlayerConstants.STATE_BUFFERING);
                break;
            case PlayerConstants.VPC_DELETE_FRAME:

                break;
        }
    }

    public void JNI_Callback(int i) {
        Log.d(TAG, "JNI_Callback::" + i);
        Message message = new Message();
        message.what = i;
        mHandler.sendMessage(message);
    }

    /**
     * 创建
     */
    public native void nativeOnCreate(ConnectivityManager cm);

    /**
     * 恢复播放
     */
    public native void nativeOnResume();

    /**
     * 暂停播放
     */
    public native void nativeOnPause();

    public native void nativeOnDelete();

    /**
     * 设置Surface
     *
     * @param surface surface
     */
    public native void nativeSetSurface(Surface surface);

    /**
     * 播放
     *
     * @param url        视频地址
     * @param start      播放开始时间
     * @param buftime    缓冲时长
     * @param videoscale 填充类型
     */
    public native void nativePlayerStart(String url, int start, int buftime, int videoscale);

    /**
     * 加载视频 暂停
     *
     * @param url        视频地址
     * @param start      播放开始时间
     * @param buftime    缓冲时长
     * @param videoscale 填充类型
     */
    public native void nativePlayerLoad(String url, int start, int buftime, int videoscale);

    /**
     * 暂停播放
     *
     * @param wait 延时时间
     */
    public native void nativePlayerPause(int wait);

    /**
     * 暂停后恢复播放
     */
    public native void nativePlayerResume();

    /**
     * 停止播放，释放播放器资源
     */
    public native void nativePlayerStop();

    /**
     * 获取已缓存时长
     *
     * @return 已缓存时长 时间单位毫秒
     */
    public native int nativePlayerGetBufTime();

    /**
     * 获得已播放时长
     *
     * @return 已播放时长 时间单位毫秒
     */
    public native int nativePlayerGetPlayPos();

    /**
     * 在收到TMPC_NOTIFY_MEDIA_INFO消息呼叫该函数获取文件总时长
     *
     * @return 总时长 时间单位毫秒
     */
    public native int nativePlayerGetLength();

    /**
     * 根据进度条设置播放时间
     *
     * @param ms 时间单位毫秒
     */
    public native void nativePlayerSeek(int ms);

    /**
     * 图像scale方式
     *
     * @param scale VIDEO_SCALE_ORIGINAL 0 原始大小
     *              VIDEO_SCALE_FIT      1 适合屏幕保持宽高比
     *              VIDEO_SCALE_FILL     2 填充屏幕，图像可能会被裁剪部分
     */
    public native int nativePlayerVideoScale(int scale);

    /**
     * @param mirror 镜像
     *               1代表镜像   0：正常
     */
    public native int nativePlayerVideoMirror(int mirror);

    /**
     * 图像scale发生变化
     *
     * @param s surface
     */
    public native void nativeSurfaceChanged(Surface s);

    /**
     * 获取视频宽度
     *
     * @return 宽度
     */
    public native int nativeGetVideoWidth();

    /**
     * 获取视频高度
     *
     * @return 高度
     */
    public native int nativeGetVideoHeight();

    /**
     * 静音
     *
     * @param mute 0：不静音 1：静音
     */
    public native int nativePlayerMute(int mute);

    private void onCreateMWPlayer(Context context) {
        Log.e(TAG, "onCreateMWPlayer");
        if (!hasCallNativeOnCreate) {
            Log.e(TAG, "nativeOnCreate");
            hasCallNativeOnCreate = true;
            ConnectivityManager cm = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
            nativeOnCreate(cm);
        }
    }

    @Override
    public void setScaleType(int scaleType) {
        mSacleType = scaleType;
        nativePlayerVideoScale(mSacleType);
    }

    @Override
    public int getCurrentTime() {
        return nativePlayerGetPlayPos();
    }

    @Override
    public int getDuration() {
        return nativePlayerGetLength();
    }

    @Override
    public int getBufferTime() {
        return nativePlayerGetBufTime();
    }

    //直播
    @Override
    public void startPlay(String uri) {
        if (TextUtils.isEmpty(uri)) {
            Log.e(TAG, "url = null");
            return;
        }
        this.url = uri;
        playTime = 0;
        Log.e(TAG, "nativePlayerStart uri = " + uri);

//        if (mSurface != null) {
            nativePlayerStart(uri, 0, 500, mSacleType);
//        } else {
//            Log.e(TAG, "surface = null");
//            isStartPlay = true;
//        }
    }

    @Override
    public void startPlay(String uri, int startTime) {
        if (uri == null) {
            Log.e(TAG, "url = null");
            return;
        }
        this.url = uri;
        playTime = startTime;
        Log.e(TAG, "nativePlayerStart uri = " + uri);

//        if (mSurface != null) {
            nativePlayerStart(uri, startTime, 500, mSacleType);
//        } else {
//            Log.e(TAG, "surface = null");
//            isStartPlay = true;
//        }
    }

    @Override
    public void stop(boolean isCallBack) {
        playState = PlayerConstants.STATE_FINISH;
        if (isCallBack) {
            onPlayerListener.onPlayState(PlayerConstants.STATE_FINISH);
        }
        nativePlayerStop();
        nativeSetSurface(null);
    }

    @Override
    public void pause() {
        playState = PlayerConstants.STATE_PAUSE;
        onPlayerListener.onPlayState(PlayerConstants.STATE_PAUSE);
        nativePlayerPause(0);
    }

    @Override
    public void resume() {
        nativePlayerResume();
    }

    @Override
    public void release() {
        isStartPlay = false;
        nativeSetSurface(null);
    }

    @Override
    public void seekTo(int ms) {
        nativePlayerSeek(ms);
    }

    @Override
    public void setOnPlayerListener(OnPlayerListener onPlayerListener) {
        this.onPlayerListener = onPlayerListener;
    }

    @Override
    public void setMute(int mute) {
        nativePlayerMute(mute);
    }

    @Override
    public int getVideoWidth() {
        return nativeGetVideoWidth();
    }

    @Override
    public int getVideoHeight() {
        return nativeGetVideoHeight();
    }

    @Override
    public void surfaceChange(Surface surface) {
        this.mSurface = surface;
        nativeSurfaceChanged(surface);
    }

    @Override
    public void setSurface(Surface surface) {
        Log.e(TAG, "setSurface");
        this.mSurface = surface;
        nativeSetSurface(surface);
        if (isStartPlay) {
            startPlay(url, playTime);
        }
    }

    @Override
    public int getPlayState() {
        return playState;
    }

    public int getBufferPercentage() {
        if (getDuration() != 0) {
            double mPercent = ((double) (nativePlayerGetBufTime() + nativePlayerGetPlayPos()) / getDuration());
            if (mPercent > 0.99444) {
                mPercent = 1;
            }
            int m = (int) (100 * mPercent);
            return getDuration() == 0 ? 0 : m;
        } else {
            return 0;
        }
    }

}

package nativeInterface;

import android.content.Context;
import android.net.ConnectivityManager;
import android.os.Message;
import android.util.Log;
import android.view.Surface;

import com.songlei.xplayer.base.CommonHandler;
import com.songlei.xplayer.obssplayer.BasePlayer;
import com.songlei.xplayer.obssplayer.ObssConstants;
import com.songlei.xplayer.obssplayer.OnObssListener;
import com.songlei.xplayer.obssplayer.PlayException;


/**
 * Created by songlei on 2018/11/08.
 */
public class playerView extends BasePlayer implements CommonHandler.HandlerCallBack {
    private static final String TAG = "playerView";
    private CommonHandler mHandler;
    private OnObssListener onObssListener;
    private Surface mSurface = null;
    // 默认全屏
    private int mSacleType = ObssConstants.VIDEO_SCALE_FILL;
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
        if (onObssListener == null) {
            return;
        }
        switch (msg.what) {
            case ObssConstants.VPC_OUT_OF_MEMORY:
                playState = ObssConstants.STATE_ERROR;
                onObssListener.onError(new PlayException(PlayException.VPC_OUT_OF_MEMORY, "内存不足"));
                break;
            case ObssConstants.VPC_NO_SOURCE_DEMUX:
                playState = ObssConstants.STATE_ERROR;
                onObssListener.onError(new PlayException(PlayException.VPC_NO_SOURCE_DEMUX, "没有数据处理器"));
                break;
            case ObssConstants.VPC_MEDIA_SPEC_ERROR:
                playState = ObssConstants.STATE_ERROR;
                onObssListener.onError(new PlayException(PlayException.VPC_MEDIA_SPEC_ERROR, "数据格式错误"));
                break;
            case ObssConstants.VPC_NO_PLAY_OBJECT:
                playState = ObssConstants.STATE_ERROR;
                onObssListener.onError(new PlayException(PlayException.VPC_NO_PLAY_OBJECT, "无播放对象"));
                break;
            case ObssConstants.VPC_NET_TIME_OUT:
                playState = ObssConstants.STATE_ERROR;
                onObssListener.onError(new PlayException(PlayException.VPC_NET_TIME_OUT, "网络超时"));
                break;
            case ObssConstants.VPC_NETWORK_ERROR:
                playState = ObssConstants.STATE_ERROR;
                onObssListener.onError(new PlayException(PlayException.VPC_NETWORK_ERROR, "网络错误"));
                break;
            case ObssConstants.VPC_START_PLAY:
                playState = ObssConstants.STATE_PLAYING;
                onObssListener.onPlayState(ObssConstants.STATE_PLAYING);
                break;
            case ObssConstants.VPC_PLAY_FINISH:
                stop(true);
                break;
            case ObssConstants.VPC_START_BUFFER_DATA:
            case ObssConstants.VPC_PLAY_BUFFER:
                playState = ObssConstants.STATE_BUFFERING;
                onObssListener.onPlayState(ObssConstants.STATE_BUFFERING);
                break;
            case ObssConstants.VPC_DELETE_FRAME:

                break;
            case ObssConstants.VPC_NOTIFY_MEDIA_INFO:
                onObssListener.onPlayState(ObssConstants.STATE_PREPARE);
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
    public native void nativePlayerLoad(String url, int start, int buftime, int videoscale);//TODO::是否能用

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
    public native int nativePlayerGetBufTime();//TODO::回调出去看

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

    @Override
    public void startPlay(String uri, int startTime) {
        if (uri == null) {
            Log.e(TAG, "url = null");
            return;
        }
        this.url = uri;
        playTime = startTime;
        Log.e(TAG, "nativePlayerStart uri = " + uri + " playTime = " + playTime);

//        if (mSurface != null) {
            nativePlayerStart(uri, startTime, 500, mSacleType);
//        } else {
//            Log.e(TAG, "surface = null");
//            isStartPlay = true;
//        }
    }

    @Override
    public void stop(boolean isCallBack) {
        playState = ObssConstants.STATE_FINISH;
        if (isCallBack) {
            onObssListener.onPlayState(ObssConstants.STATE_FINISH);
        }
        playTime = 0;
        nativePlayerStop();
        nativeSetSurface(null);
    }

    @Override
    public void pause() {
        playState = ObssConstants.STATE_PAUSE;
        onObssListener.onPlayState(ObssConstants.STATE_PAUSE);
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
    public void setOnObssListener(OnObssListener onObssListener) {
        this.onObssListener = onObssListener;
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
//        if (isStartPlay) {
//            startPlay(url, playTime);
//        }
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

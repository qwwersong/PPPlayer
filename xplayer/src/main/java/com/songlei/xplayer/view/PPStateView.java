package com.songlei.xplayer.view;

import android.content.Context;
import android.media.AudioManager;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.InflateException;
import android.view.Surface;
import android.view.View;

import com.songlei.xplayer.PlayerConstants;
import com.songlei.xplayer.PlayerManager;
import com.songlei.xplayer.R;
import com.songlei.xplayer.base.Option;
import com.songlei.xplayer.listener.PlayerListener;
import com.songlei.xplayer.player.IjkPlayer;
import com.songlei.xplayer.player.ObssPlayer;
import com.songlei.xplayer.player.PlayerFactory;
import com.songlei.xplayer.util.CommonUtil;

/**
 * Created by songlei on 2019/07/02.
 */
public abstract class PPStateView extends PPTextureRenderView {
    public static final int STATE_NO_PLAY = PlayerConstants.STATE_NO_PLAY;
    public static final int STATE_PLAYING = PlayerConstants.STATE_PLAYING;
    public static final int STATE_PREPARE = PlayerConstants.STATE_PREPARE;
    public static final int STATE_PAUSE = PlayerConstants.STATE_PAUSE;
    public static final int STATE_COMPLETE = PlayerConstants.STATE_COMPLETE;
    public static final int STATE_BUFFERING = PlayerConstants.STATE_BUFFERING;
    public static final int STATE_ERROR = PlayerConstants.STATE_ERROR;

    //上下文
    protected Context mContext;
    //屏幕宽度
    protected int mScreenWidth;
    //屏幕高度
    protected int mScreenHeight;
    //音频焦点监听
    protected AudioManager mAudioManager;
    //播放地址
    protected String mUrl;
    //播放器控制类
    protected PlayerManager mPlayerManager;
    //临时播放器控制类
    protected PlayerManager mTmpPlayerManager;
    //当前的播放状态
    protected int mCurrentState;
//    //当前播放时间
//    protected long mCurrentPosition;
    //是否播放过
    protected boolean mHadPlay = false;
    //从哪个开始播放
    protected long mSeekOnStart = -1;
    //用于记录列表播放，播放器在列表中的位置
    protected int mPlayerPosition = -22;

    public PPStateView(Context context) {
        super(context);
        init(context);
    }

    public PPStateView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    public PPStateView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context);
    }

    protected void init(Context context){
        mContext = context;

        initInflate(mContext);

        mTextureViewContainer = findViewById(R.id.surface_container);

        mScreenWidth = mContext.getResources().getDisplayMetrics().widthPixels;
        mScreenHeight = mContext.getResources().getDisplayMetrics().heightPixels;

        mAudioManager = (AudioManager) mContext.getApplicationContext().getSystemService(Context.AUDIO_SERVICE);
        mAudioManager.requestAudioFocus(onAudioFocusChangeListener, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);

    }

    protected void initPlayer(){
        mPlayerManager = PlayerManager.getInstance();
        mPlayerManager.initPlayer(mContext);
        mPlayerManager.setPlayerListener(playerListener);
    }

    protected void initInflate(Context context){
        try {
            View.inflate(context, getLayoutId(), this);
        } catch (InflateException e) {
            e.printStackTrace();
        }
    }

    /**
     * 监听是否有外部其他多媒体开始播放
     */
    protected AudioManager.OnAudioFocusChangeListener onAudioFocusChangeListener = new AudioManager.OnAudioFocusChangeListener() {
        @Override
        public void onAudioFocusChange(int focusChange) {
            switch (focusChange) {
                case AudioManager.AUDIOFOCUS_GAIN:
//                    onGankAudio();
                    break;
                case AudioManager.AUDIOFOCUS_LOSS:
//                    onLossAudio();
                    break;
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
//                    onLossTransientAudio();
                    break;
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
//                    onLossTransientCanDuck();
                    break;
            }
        }
    };

    protected void setUrl(String url){
        mUrl = url;
    }

    protected void prepare(){
        if (mPlayerManager == null) {
            return;
        }
        if (!TextUtils.isEmpty(mUrl)) {
            mPlayerManager.prepare(mUrl);
        }
        mPlayerManager.setPlayerPosition(mPlayerPosition);
    }

    protected void start(){
        if (mPlayerManager != null) {
            mPlayerManager.start();
        }
    }

    protected void pause(){
        if (mPlayerManager != null) {
            mPlayerManager.pause();
        }
    }

    protected void resume(){
        if (mPlayerManager != null) {
            mPlayerManager.resume();
        }
    }

    public void stop(){
        if (mPlayerManager != null) {
            mPlayerManager.stop();
        }
    }

    public void release(){
        if (mPlayerManager != null) {
            mPlayerManager.release();
        }
    }

    protected void seekTo(int time){
        if (mPlayerManager != null) {
            mPlayerManager.seekTo(time);
        }
    }

    @Override
    protected void setDisplay(Surface surface) {
        Log.e("xxx", "setDisplay mPlayerManager = " + mPlayerManager);
        if (mPlayerManager != null) {
            mPlayerManager.setSurface(surface);
        }
    }

    @Override
    protected void surfaceChanged(Surface surface) {
        if (mPlayerManager != null) {
            mPlayerManager.surfaceChanged(surface);
        }
    }

    @Override
    protected void releaseSurface(Surface surface) {

    }

    @Override
    public int getCurrentVideoWidth() {
        if (mPlayerManager != null) {
            return mPlayerManager.getCurrentVideoWidth();
        }
        return 0;
    }

    @Override
    public int getCurrentVideoHeight() {
        if (mPlayerManager != null) {
            return mPlayerManager.getCurrentVideoHeight();
        }
        return 0;
    }

    @Override
    public int getVideoSarNum() {
        return 0;
    }

    @Override
    public int getVideoSarDen() {
        return 0;
    }

    protected PlayerListener playerListener = new PlayerListener() {

        @Override
        public void onBufferingUpdate(int percent) {
            onBufferedUpdate(percent);
        }

        @Override
        public void onPlayerState(int state) {
            switch (state) {
                case STATE_NO_PLAY:
                    Log.e("xxx", "未播放");
                    mCurrentState = STATE_NO_PLAY;
                    onStateLayout(mCurrentState);
                    break;
                case STATE_PREPARE:
                    Log.e("xxx", "准备中....");
                    mCurrentState = STATE_PREPARE;
                    Log.e("xxx", "mSeekOnStart = " + mSeekOnStart);
                    if (mSeekOnStart > 0) {
                        mPlayerManager.seekTo(mSeekOnStart);
                        mSeekOnStart = 0;
                    }
                    addTextureView();
                    if (Option.getPlayerType() != Option.PLAYER_OBSS) {
                        start();
                    }
                    onStateLayout(mCurrentState);
                    break;
                case STATE_PLAYING:
                    Log.e("xxx", "播放中....");
                    mCurrentState = STATE_PLAYING;
                    mHadPlay = true;
                    onStateLayout(mCurrentState);
                    break;
                case STATE_PAUSE:
                    Log.e("xxx", "暂停");
                    mCurrentState = STATE_PAUSE;
                    onStateLayout(mCurrentState);
                    break;
                case STATE_COMPLETE:
                    Log.e("xxx", "播放结束");
                    mCurrentState = STATE_COMPLETE;
                    onStateLayout(mCurrentState);
                    break;
                case STATE_BUFFERING:
                    Log.e("xxx", "缓冲中");
                    mCurrentState = STATE_BUFFERING;
                    onStateLayout(mCurrentState);
                    break;
            }
        }

        @Override
        public void onPlayerError(int error, int extra) {
            Log.e("xxx", "播放异常 error = " + error);
            mCurrentState = STATE_ERROR;
            onErrorLayout(error);
        }
    };

    public int getCurrentPosition(){
        int position = 0;
        if (mCurrentState == STATE_PLAYING || mCurrentState == STATE_PAUSE) {
            if (mPlayerManager != null) {
                position = (int) mPlayerManager.getCurrentPosition();
            }
        }
//        if (position == 0 && mCurrentPosition > 0) {
//            return (int) mCurrentPosition;
//        }
        return position;
    }

    public int getDuration() {
        if (mPlayerManager != null) {
            return (int) mPlayerManager.getDuration();
        }
        return 0;
    }

    public int getBufferedPercentage() {
        if (mPlayerManager != null) {
            return mPlayerManager.getBufferedPercentage();
        }
        return 0;
    }

    protected void setPlayPosition(long currentPosition){
        if (mPlayerManager != null) {
            mPlayerManager.setPlayPosition(currentPosition);
        }
    }

    protected boolean isPlaying(){
        boolean isPlaying = mCurrentState == STATE_PLAYING;
        return isPlaying;
    }

    public void setRenderType(int renderType){
        Option.setRenderType(renderType);
    }

    public void setPlayerType(int playerType){
        switch (playerType) {
            case Option.PLAYER_IJK:
                PlayerFactory.setPlayer(IjkPlayer.class);
                Option.setPlayerType(Option.PLAYER_IJK);
                break;
            case Option.PLAYER_OBSS:
                PlayerFactory.setPlayer(ObssPlayer.class);
                Option.setPlayerType(Option.PLAYER_OBSS);
                break;
            case Option.PLAYER_SYSTEM:

                break;
        }
    }

    public void setSpeedPlaying(float speed, boolean soundTouch){
        if (mPlayerManager != null) {
            mPlayerManager.setSpeedPlaying(speed, soundTouch);
        }
    }

    public void setPlayerPosition(int mPlayerPosition){
        this.mPlayerPosition = mPlayerPosition;
    }

//    public void setMediaCodec(boolean isMediaCodec){
//        Option.setPlayerMediaCodec(isMediaCodec);
//    }

    protected Context getActivityContext(){
        return CommonUtil.getActivityContext(getContext());
    }

    /*
      =========================================抽象接口===============================================
     */

    /**
     * 当前UI
     */
    protected abstract int getLayoutId();

    /**
     * 显示对应状态的布局
     * @param state 播放器状态
     */
    protected abstract void onStateLayout(int state);

    protected abstract void onErrorLayout(int errorCode);

    protected abstract void onBufferedUpdate(int percent);
}

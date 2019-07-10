package com.songlei.xplayer.view;

import android.content.Context;
import android.media.AudioManager;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.InflateException;
import android.view.Surface;
import android.view.View;

import com.songlei.xplayer.R;
import com.songlei.xplayer.base.Option;
import com.songlei.xplayer.listener.PlayerListener;
import com.songlei.xplayer.player.PlayerManager;

/**
 * Created by songlei on 2019/07/02.
 */
public abstract class PPStateView extends PPTextureRenderView {
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
        mPlayerManager = new PlayerManager(context);
        Option.setPlayerType(Option.PLAYER_OBSS);
        mPlayerManager.initPlayer(null);
        mPlayerManager.setPlayerListener(playerListener);
    }

    protected void initInflate(Context context){
        try {
            View.inflate(context, getLayoutId(), this);
        } catch (InflateException e) {
            e.printStackTrace();
        }
    }

    protected void setUrl(String url){
        mUrl = url;
    }

    protected void prepare(){
        if (!TextUtils.isEmpty(mUrl)) {
            mPlayerManager.prepare(mUrl);
        }
    }

    @Override
    protected void setDisplay(Surface surface) {
        mPlayerManager.setSurface(surface);
    }

    protected void start(){
        mPlayerManager.start();
    }

    @Override
    protected void releaseSurface(Surface surface) {

    }

    @Override
    public int getCurrentVideoWidth() {
        Log.e("xxx", "PPStateView getCurrentVideoWidth = " + mPlayerManager.getCurrentVideoWidth());
        return mPlayerManager.getCurrentVideoWidth();
    }

    @Override
    public int getCurrentVideoHeight() {
        Log.e("xxx", "PPStateView getCurrentVideoHeight = " + mPlayerManager.getCurrentVideoHeight());
        return mPlayerManager.getCurrentVideoHeight();
    }

    @Override
    public int getVideoSarNum() {
        return 0;
    }

    @Override
    public int getVideoSarDen() {
        return 0;
    }

    private PlayerListener playerListener = new PlayerListener() {
        @Override
        public void onCompletion() {

        }

        @Override
        public void onPrepared() {
            addTextureView();
        }

        @Override
        public void onSeekComplete() {

        }

        @Override
        public boolean onError(int what, int extra) {
            return false;
        }

        @Override
        public boolean onInfo(int what, int extra) {
            return false;
        }

        @Override
        public void onVideoSizeChanged(int width, int height) {

        }
    };

    /*
      =========================================抽象接口===============================================
     */

    /**
     * 当前UI
     */
    public abstract int getLayoutId();
}

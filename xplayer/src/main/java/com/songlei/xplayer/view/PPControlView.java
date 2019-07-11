package com.songlei.xplayer.view;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import com.songlei.xplayer.R;
import com.songlei.xplayer.util.CommonUtil;

import java.util.Timer;
import java.util.TimerTask;

/**
 * 处理控件操作、界面显示状态
 * Created by songlei on 2019/07/02.
 */
public abstract class PPControlView extends PPStateView implements View.OnClickListener,
        View.OnTouchListener, SeekBar.OnSeekBarChangeListener {
    //==============================布局控件=============================
    //播放按键
    protected View mStartButton;
    //封面
    protected View mThumbImageView;
    //loading view
    protected View mLoadingProgressBar;
    //进度条
    protected SeekBar mProgressBar;
    //全屏按键
    protected ImageView mFullscreenButton;
    //返回按键
    protected ImageView mBackButton;
    //锁定图标
    protected ImageView mLockScreen;
    //时间显示
    protected TextView mCurrentTimeTextView, mTotalTimeTextView;
    //title
    protected TextView mTitleTextView;
    //顶部和底部区域
    protected LinearLayout mTopContainer, mBottomContainer;
    //封面父布局
    protected RelativeLayout mThumbImageViewLayout;
    //底部进度调
    protected ProgressBar mBottomProgressBar;
    //==============================控制参数=============================
    //是否隐藏虚拟按键
    protected boolean mHideKey = true;
    //是否触摸进度条
    protected boolean mTouchProgressBar;
    //正在seek
    protected boolean mHadSeekTouch = false;
    //进度定时器
    protected Timer updateProcessTimer;
    //触摸显示消失定时
    protected Timer mDismissControlViewTimer;
    //进度条定时器
    protected ProgressTimerTask mProgressTimerTask;


    public PPControlView(Context context) {
        super(context);
    }

    public PPControlView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public PPControlView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    protected void init(Context context) {
        super.init(context);

        mStartButton = findViewById(R.id.start);
        mTitleTextView = findViewById(R.id.title);
        mBackButton = findViewById(R.id.back);
        mFullscreenButton = findViewById(R.id.fullscreen);
        mProgressBar = findViewById(R.id.progress);
        mCurrentTimeTextView = findViewById(R.id.current);
        mTotalTimeTextView = findViewById(R.id.total);
        mBottomContainer = findViewById(R.id.layout_bottom);
        mTopContainer = findViewById(R.id.layout_top);
        mBottomProgressBar = findViewById(R.id.bottom_progressbar);
        mThumbImageViewLayout = findViewById(R.id.thumb);
        mLockScreen = findViewById(R.id.lock_screen);

        mLoadingProgressBar = findViewById(R.id.loading);

        if (mStartButton != null) {
            mStartButton.setOnClickListener(this);
            updateStartImage();
        }

//        if (mFullscreenButton != null) {
//            mFullscreenButton.setOnClickListener(this);
//            mFullscreenButton.setOnTouchListener(this);
//        }

        if (mProgressBar != null) {
            mProgressBar.setOnSeekBarChangeListener(this);
        }

        if (mBottomContainer != null) {
            mBottomContainer.setOnClickListener(this);
        }

        if (mTextureViewContainer != null) {
            mTextureViewContainer.setOnClickListener(this);
            mTextureViewContainer.setOnTouchListener(this);
        }

//        if (mProgressBar != null) {
//            mProgressBar.setOnTouchListener(this);
//        }

        if (mThumbImageViewLayout != null) {
            mThumbImageViewLayout.setVisibility(GONE);
            mThumbImageViewLayout.setOnClickListener(this);
        }
//        if (mThumbImageView != null && !mIfCurrentIsFullscreen && mThumbImageViewLayout != null) {
//            mThumbImageViewLayout.removeAllViews();
//            resolveThumbImage(mThumbImageView);
//        }

        if (mBackButton != null)
            mBackButton.setOnClickListener(this);

        if (mLockScreen != null) {
            mLockScreen.setVisibility(GONE);
            mLockScreen.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
//                    if (mCurrentState == CURRENT_STATE_AUTO_COMPLETE ||
//                            mCurrentState == CURRENT_STATE_ERROR) {
//                        return;
//                    }
//                    lockTouchLogic();
//                    if (mLockClickListener != null) {
//                        mLockClickListener.onClick(v, mLockCurScreen);
//                    }
                }
            });
        }
    }

    @Override
    public void onClick(View v) {
        if (v.getId() == R.id.start) {
            clickStartIcon();
        }
    }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        return false;
    }

    public void clickStartIcon() {
        if (TextUtils.isEmpty(mUrl)) {
            Log.e("xxx", "播放地址为空");
            return;
        }
        if (mCurrentState == STATE_NO_PLAY) {
            prepare();
            start();
        } else if (mCurrentState == STATE_PLAYING) {
            pause();
        } else if (mCurrentState == STATE_PAUSE) {
            resume();
        } else if (mCurrentState == STATE_COMPLETE) {
            prepare();
            start();
        }
    }

    protected void updateStartImage() {
        ImageView imageView = (ImageView) mStartButton;
        if (mCurrentState == STATE_PLAYING) {
            imageView.setImageResource(R.drawable.ic_pause);
        } else {
            imageView.setImageResource(R.drawable.ic_play);
        }
    }

    @Override
    public void onStateLayout(int state) {
        switch (state) {
            case STATE_NO_PLAY:
                stopProgressTimer();
                break;
            case STATE_PREPARE:
                mLoadingProgressBar.setVisibility(VISIBLE);
                break;
            case STATE_PLAYING:
                startProgressTimer();
                mLoadingProgressBar.setVisibility(INVISIBLE);
                break;
            case STATE_PAUSE:

                break;
            case STATE_COMPLETE:
                stopProgressTimer();
                mLoadingProgressBar.setVisibility(INVISIBLE);
                break;
            case STATE_BUFFERING:
                Log.e("xxx", "爱地魔力转圈圈");
                mLoadingProgressBar.setVisibility(VISIBLE);
                break;
        }
        updateStartImage();
    }

    public void startProgressTimer(){
        stopProgressTimer();
        updateProcessTimer = new Timer();
        mProgressTimerTask = new ProgressTimerTask();
        updateProcessTimer.schedule(mProgressTimerTask, 0, 300);
    }

    public void stopProgressTimer(){
        if (updateProcessTimer != null) {
            updateProcessTimer.cancel();
            updateProcessTimer = null;
        }
        if (mProgressTimerTask != null) {
            mProgressTimerTask.cancel();
            mProgressTimerTask = null;
        }
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {

    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        mHadSeekTouch = true;
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        int time = seekBar.getProgress() * getDuration() / 100;
        seekTo(time);
        mHadSeekTouch = false;
    }

    private class ProgressTimerTask extends TimerTask {

        @Override
        public void run() {
            if (mCurrentState == STATE_PLAYING || mCurrentState == STATE_PAUSE) {
                new Handler(Looper.getMainLooper()).post(new Runnable() {
                    @Override
                    public void run() {
                        int position = getCurrentPosition();
                        int duration = getDuration();
                        int progress = position * 100 / (duration == 0 ? 1 : duration);
                        setProgressAndTime(progress, 0, position, duration);
                    }
                });
            }
        }
    }

    protected void setProgressAndTime(int progress, int secProgress, int currentTime, int totalTime){
        if (mProgressBar == null || mTotalTimeTextView == null || mCurrentTimeTextView == null) {
            return;
        }
        if (mHadSeekTouch) {
            return;
        }
        if (!mTouchProgressBar) {
            mProgressBar.setProgress(progress);
        }
        mTotalTimeTextView.setText(CommonUtil.stringForTime(totalTime));
        if (currentTime > 0 ) {
            mCurrentTimeTextView.setText(CommonUtil.stringForTime(currentTime));
        }
    }

//    private SeekBar.OnSeekBarChangeListener seekBarChangeListener = new SeekBar.OnSeekBarChangeListener() {
//        @Override
//        public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
//
//        }
//
//        @Override
//        public void onStartTrackingTouch(SeekBar seekBar) {
//            mHadSeekTouch = true;
//            Log.e("xxx", "=========onStartTrackingTouch mHadSeekTouch = " + mHadSeekTouch);
//        }
//
//        @Override
//        public void onStopTrackingTouch(SeekBar seekBar) {
//            int time = seekBar.getProgress() * getDuration() / 100;
//            seekTo(time);
//            mHadSeekTouch = false;
//        }
//    };

}

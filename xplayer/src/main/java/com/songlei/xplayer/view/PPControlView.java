package com.songlei.xplayer.view;

import android.app.Activity;
import android.content.Context;
import android.media.AudioManager;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewParent;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import com.songlei.xplayer.R;
import com.songlei.xplayer.util.CommonUtil;
import com.songlei.xplayer.util.NetUtil;

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
    //==============================onTouch参数==========================
    //触摸的X
    protected float mDownX;
    //触摸的Y
    protected float mDownY;
    //移动的Y
    protected float mMoveY;

    //触摸的是否进度条
    protected boolean mTouchingProgressBar = false;
    //是否改变音量
    protected boolean mChangeVolume = false;
    //是否改变播放进度
    protected boolean mChangePosition = false;
    //触摸显示虚拟按键
    protected boolean mShowVKey = false;
    //是否改变亮度
    protected boolean mBrightness = false;
    //是否首次触摸
    protected boolean mFirstTouch = false;

    //手动改变滑动的位置
    protected int mSeekTimePosition;
    //手势偏差值
    protected int mThreshold = 50;
    //手指放下时播放的位置
    protected int mDownPosition;
    //手动滑动的起始偏移位置
    protected int mSeekEndOffset;
    //手势调节音量的大小
    protected int mGestureDownVolume;
    //触摸滑动进度的比例系数
    protected float mSeekRatio = 1;

    //当前是否全屏
    public boolean mIfCurrentIsFullScreen = false;
    //是否支持非全屏滑动触摸有效
    protected boolean mIsTouchWidget = true;
    //是否支持全屏滑动触摸有效
    protected boolean mIsTouchWidgetFull = true;

    //亮度
    protected float mBrightnessData = -1;
    //==============================控制参数=============================
    //是否隐藏虚拟按键
    protected boolean mHideKey = true;
    //是否触摸进度条
    protected boolean mTouchProgressBar;
    //正在seek
    protected boolean mHadSeekTouch = false;
    //锁定屏幕点击
    protected boolean mLockCurScreen;
    //进度定时器
    protected Timer updateProcessTimer;
    //进度条定时器
    protected ProgressTimerTask mProgressTimerTask;
    //触摸显示消失定时
    protected Timer mDismissControlViewTimer;
    //触摸显示消失定时任务
    protected DismissControlViewTimerTask mDismissControlViewTimerTask;

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
            mProgressBar.setOnTouchListener(this);
        }

        if (mBottomContainer != null) {
            mBottomContainer.setOnClickListener(this);
        }

        if (mTextureViewContainer != null) {
            mTextureViewContainer.setOnClickListener(this);
            mTextureViewContainer.setOnTouchListener(this);
        }

        if (mThumbImageViewLayout != null) {
            mThumbImageViewLayout.setVisibility(GONE);
            mThumbImageViewLayout.setOnClickListener(this);
        }

        if (mBackButton != null) {
            mBackButton.setOnClickListener(this);
        }

        if (mLockScreen != null) {
            mLockScreen.setVisibility(GONE);
            mLockScreen.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (mCurrentState == STATE_COMPLETE) {
                        return;
                    }
                    lockTouchLogic();
                }
            });
        }

        mSeekEndOffset = CommonUtil.dip2px(getContext(), 50);
    }

    /**
     * ===================================按键逻辑===========================================
     */
    @Override
    public void onClick(View v) {
        int id = v.getId();
        if (id == R.id.start) {
            clickStartIcon();
        } else if (id == R.id.surface_container) {
            startDismissControlViewTimer();
        }
    }

    protected GestureDetector gestureDetector = new GestureDetector(getContext(), new GestureDetector.SimpleOnGestureListener() {
        @Override
        public boolean onDoubleTap(MotionEvent e) {
            Log.e("xxx", "onDoubleTap");
            if (mHadPlay) {
                clickStartIcon();
            }
            return super.onDoubleTap(e);
        }

        @Override
        public boolean onSingleTapConfirmed(MotionEvent e) {
            if (!mChangePosition && !mChangeVolume && !mBrightness) {
                Log.e("xxx", "onSingleTapConfirmed");
                onClickUiToggle();
            }
            return super.onSingleTapConfirmed(e);
        }
    });

    @Override
    public boolean onTouch(View v, MotionEvent event) {

        int id = v.getId();
        float x = event.getX();
        float y = event.getY();

        if (id == R.id.surface_container) {
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    mDownX = x;
                    mDownY = y;
                    mChangeVolume = false;
                    mChangePosition = false;
                    mBrightness = false;
                    mFirstTouch = true;
                    break;
                case MotionEvent.ACTION_MOVE:
                    float deltaX = x - mDownX;
                    float deltaY = y - mDownY;

                    float absDeltaX = Math.abs(deltaX);
                    float absDeltaY = Math.abs(deltaY);

                    if ((mIfCurrentIsFullScreen && mIsTouchWidgetFull)
                            || (mIsTouchWidget && !mIfCurrentIsFullScreen)) {
                        if (!mChangePosition && !mChangeVolume && !mBrightness) {

                            touchSurfaceMoveLogic(absDeltaX, absDeltaY);
                        }
                    }
                    touchSurfaceMove(deltaX, deltaY, y);

                    break;
                case MotionEvent.ACTION_UP:
                    startDismissControlViewTimer();
                    startProgressTimer();

                    touchSurfaceUp();
                    break;
            }
        } else if (id == R.id.progress) {
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    cancelDismissControlViewTimer();
                case MotionEvent.ACTION_MOVE:
                    stopProgressTimer();
                    ViewParent vpdown = getParent();
                    while (vpdown != null) {
                        vpdown.requestDisallowInterceptTouchEvent(true);
                        vpdown = vpdown.getParent();
                    }
                    break;
                case MotionEvent.ACTION_UP:
                    startDismissControlViewTimer();
                    startProgressTimer();
                    ViewParent vpup = getParent();
                    while (vpup != null) {
                        vpup.requestDisallowInterceptTouchEvent(false);
                        vpup = vpup.getParent();
                    }
                    mBrightnessData = -1f;
                    break;
            }
        }
        gestureDetector.onTouchEvent(event);

        return false;
    }

    protected void touchSurfaceMoveLogic(float absDeltaX, float absDeltaY){
        int curWidth = CommonUtil.getCurrentScreenLand(CommonUtil.getActivityContext(getContext())) ? mScreenHeight : mScreenWidth;

        Log.e("xxx", "absDeltaX = " + absDeltaX + " absDeltaY = " + absDeltaY);
        if (absDeltaX > mThreshold || absDeltaY > mThreshold) {//超过偏差值，认定滑动
            //因为会多次调用，不能用else if会两个条件都满足
            //TODO::简化逻辑表达式
            if (absDeltaX >= mThreshold) {//认定横向滑动
                Log.e("xxx", "横向滑动");
                mChangePosition = true;
                mDownPosition = getCurrentPosition();
            } else {//认定纵向滑动
                Log.e("xxx", "纵向滑动");
                int screenHeight = CommonUtil.getScreenHeight(getContext());
                Log.e("xxx", "screenHeight = " + screenHeight + " mDownY = " + mDownY + " 差值 = " + Math.abs(screenHeight - mDownY));
                Log.e("xxx", "mSeekEndOffset = " + mSeekEndOffset);
                boolean noEnd = Math.abs(screenHeight - mDownY) > mSeekEndOffset;//TODO::这个表达式是什么意思?
                if (mFirstTouch) {
                    mBrightness = (mDownX < curWidth * 0.5f) && noEnd;//是否调节亮度，在左边屏幕且？
                    mFirstTouch = false;
                }
                if (!mBrightness) {
                    mChangeVolume = noEnd;//是否调节音量
                    mGestureDownVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
                }
            }
        }
    }

    protected void touchSurfaceMove(float deltaX, float deltaY, float y){
        int curWidth = CommonUtil.getCurrentScreenLand(CommonUtil.getActivityContext(getContext())) ? mScreenHeight : mScreenWidth;
        int curHeight = CommonUtil.getCurrentScreenLand(CommonUtil.getActivityContext(getContext())) ? mScreenWidth : mScreenHeight;

        if (mChangePosition) {//调节播放进度
            int duration = getDuration();

            mSeekTimePosition = (int) (mDownPosition + (deltaX * duration / curWidth) / mSeekRatio);
            if (mSeekTimePosition > duration) {
                mSeekTimePosition = duration;
            }
            String seekTime = CommonUtil.stringForTime(mSeekTimePosition);
            String totalTime = CommonUtil.stringForTime(duration);
            showProgressDialog(deltaX, seekTime, mSeekTimePosition, totalTime, duration);
        } else if (mChangeVolume) {//调节音量
            deltaY = -deltaY;
            int max = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
            int deltaV = (int) (max * deltaY * 3 / curHeight);
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, mGestureDownVolume + deltaV, 0);
            int volumePercent = (int) (mGestureDownVolume * 100 / max + deltaY * 3 * 100 / curHeight);
            showVolumeDialog(-deltaY, volumePercent);
        } else if (mBrightness) {//调节亮度
            float percent = (-deltaY / curHeight);

            mBrightnessData = ((Activity) (mContext)).getWindow().getAttributes().screenBrightness;
            if (mBrightnessData <= 0.00f) {
                mBrightnessData = 0.50f;
            } else if (mBrightnessData < 0.01f) {
                mBrightnessData = 0.01f;
            }
            WindowManager.LayoutParams lpa = ((Activity) (mContext)).getWindow().getAttributes();
            lpa.screenBrightness = mBrightnessData + percent;
            if (lpa.screenBrightness > 1.0f) {
                lpa.screenBrightness = 1.0f;
            } else if (lpa.screenBrightness < 0.01f) {
                lpa.screenBrightness = 0.01f;
            }
            showBrightnessDialog(lpa.screenBrightness);
            ((Activity) (mContext)).getWindow().setAttributes(lpa);

            mDownY = y;
        }
    }

    protected void touchSurfaceUp(){
        dismissProgressDialog();
        dismissVolumeDialog();
        dismissBrightnessDialog();
        if (mChangePosition && (mCurrentState == STATE_PLAYING || mCurrentState == STATE_PAUSE)) {
            mLoadingProgressBar.setVisibility(VISIBLE);
            seekTo(mSeekTimePosition);
            int duration = getDuration();
            int progress = mSeekTimePosition * 100 / (duration == 0 ? 1 : duration);
            if (mProgressBar != null) {
                mProgressBar.setProgress(progress);
            }
        } else if (mChangeVolume) {
            //回调
        } else if (mBrightness) {
            //回调
        }
    }

    public void clickStartIcon() {
        if (TextUtils.isEmpty(mUrl)) {
            return;
        }
        if (mCurrentState == STATE_NO_PLAY) {
            initPlayer();
            startPlayLogic();
        } else if (mCurrentState == STATE_PLAYING) {
            pause();
        } else if (mCurrentState == STATE_PAUSE) {
            resume();
        } else if (mCurrentState == STATE_COMPLETE) {
            initPlayer();
            startPlayLogic();
        } else if (mCurrentState == STATE_ERROR) {

        }
    }

    protected void startPlayLogic(){
        if (!NetUtil.isWifiConnected(mContext)) {
            showWifiDialog();
            return;
        }
        prepare();
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
        mLoadingProgressBar.setVisibility(VISIBLE);
        int time = seekBar.getProgress() * getDuration() / 100;
        seekTo(time);
        mHadSeekTouch = false;
    }

    /**
     * ===================================UI状态显示逻辑===========================================
     */
    @Override
    protected void onStateLayout(int state) {
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
                startDismissControlViewTimer();
                break;
            case STATE_PAUSE:
                showAllWidget();
                cancelDismissControlViewTimer();
                break;
            case STATE_COMPLETE:
                showAllWidget();
                stopProgressTimer();
                mProgressBar.setProgress(0);
                mCurrentTimeTextView.setText(CommonUtil.stringForTime(0));
                mLoadingProgressBar.setVisibility(INVISIBLE);
                break;
            case STATE_BUFFERING:
                mLoadingProgressBar.setVisibility(VISIBLE);
                break;
        }
        updateStartImage();
    }

    @Override
    protected void onErrorLayout(int errorCode) {
        showNetError(errorCode);
        setViewShowState(mBottomContainer, GONE);
    }

    @Override
    protected void onBufferedUpdate(int percent) {
        if (percent != 0) {
//            setTextAndProgress(percent);
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

    //开始更新进度条
    public void startProgressTimer(){
        stopProgressTimer();
        updateProcessTimer = new Timer();
        mProgressTimerTask = new ProgressTimerTask();
        updateProcessTimer.schedule(mProgressTimerTask, 0, 300);
    }

    //停止更新进度条
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

    private class ProgressTimerTask extends TimerTask {

        @Override
        public void run() {
            if (mCurrentState == STATE_PLAYING || mCurrentState == STATE_PAUSE) {
                new Handler(Looper.getMainLooper()).post(new Runnable() {
                    @Override
                    public void run() {
                        setTextAndProgress(0);
                    }
                });
            }
        }
    }

    protected void setTextAndProgress(int secProgress){
        int position = getCurrentPosition();
        int duration = getDuration();
        int progress = position * 100 / (duration == 0 ? 1 : duration);
        setProgressAndTime(progress, secProgress, position, duration);
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
        //只有播放地址为mp4，才会有回调播放器BufferingUpdate
        if (getBufferedPercentage() > 0) {
            secProgress = getBufferedPercentage();
        }
        if (secProgress != 0) {
            mProgressBar.setSecondaryProgress(secProgress);
        }

        mTotalTimeTextView.setText(CommonUtil.stringForTime(totalTime));
        if (currentTime > 0 ) {
            mCurrentTimeTextView.setText(CommonUtil.stringForTime(currentTime));
        }
    }

    protected void startDismissControlViewTimer(){
        cancelDismissControlViewTimer();
        mDismissControlViewTimer = new Timer();
        mDismissControlViewTimerTask = new DismissControlViewTimerTask();
        mDismissControlViewTimer.schedule(mDismissControlViewTimerTask, 2500);
    }

    protected void cancelDismissControlViewTimer(){
        if (mDismissControlViewTimer != null) {
            mDismissControlViewTimer.cancel();
            mDismissControlViewTimer = null;
        }
        if (mDismissControlViewTimerTask != null) {
            mDismissControlViewTimerTask.cancel();
            mDismissControlViewTimerTask = null;
        }
    }

    private class DismissControlViewTimerTask extends TimerTask {

        @Override
        public void run() {
            if (mCurrentState != STATE_NO_PLAY && mCurrentState != STATE_COMPLETE) {
                new Handler(Looper.getMainLooper()).post(new Runnable() {
                    @Override
                    public void run() {
                        hideAllWidget();
                        setViewShowState(mLockScreen, GONE);
                    }
                });
            }
        }
    }

    public void setViewShowState(View view, int visibility){
        if (view != null) {
            view.setVisibility(visibility);
        }
    }

    protected void lockTouchLogic(){
        if (mLockCurScreen) {
            mLockCurScreen = false;
            mLockScreen.setImageResource(R.drawable.ic_unlock);
            showAllWidget();
        } else {
            mLockCurScreen = true;
            mLockScreen.setImageResource(R.drawable.ic_lock);
            hideAllWidget();
        }
    }

    protected abstract void hideAllWidget();

    protected abstract void showAllWidget();

    protected abstract void onClickUiToggle();

    protected abstract void showWifiDialog();

    protected abstract void showProgressDialog(float deltaX,
                                               String seekTime, int seekTimePosition,
                                               String totalTime, int totalTimeDuration);

    protected abstract void showVolumeDialog(float deltaY, int volumePercent);

    protected abstract void showBrightnessDialog(float percent);

    protected abstract void dismissProgressDialog();

    protected abstract void dismissVolumeDialog();

    protected abstract void dismissBrightnessDialog();

    protected abstract void showNetError(int errorCode);

}

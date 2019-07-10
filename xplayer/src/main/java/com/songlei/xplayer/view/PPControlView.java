package com.songlei.xplayer.view;

import android.content.Context;
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

import java.util.Timer;

/**
 * 处理控件操作、界面显示状态
 * Created by songlei on 2019/07/02.
 */
public abstract class PPControlView extends PPStateView implements View.OnClickListener,
        View.OnTouchListener {
    //是否隐藏虚拟按键
    protected boolean mHideKey = true;
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

    //进度定时器
    protected Timer updateProcessTimer;

    //触摸显示消失定时
    protected Timer mDismissControlViewTimer;

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
        mTitleTextView = (TextView) findViewById(R.id.title);
        mBackButton = (ImageView) findViewById(R.id.back);
        mFullscreenButton = (ImageView) findViewById(R.id.fullscreen);
        mProgressBar = (SeekBar) findViewById(R.id.progress);
        mCurrentTimeTextView = (TextView) findViewById(R.id.current);
        mTotalTimeTextView = (TextView) findViewById(R.id.total);
        mBottomContainer = findViewById(R.id.layout_bottom);
        mTopContainer = findViewById(R.id.layout_top);
        mBottomProgressBar = (ProgressBar) findViewById(R.id.bottom_progressbar);
        mThumbImageViewLayout = (RelativeLayout) findViewById(R.id.thumb);
        mLockScreen = (ImageView) findViewById(R.id.lock_screen);

        mLoadingProgressBar = findViewById(R.id.loading);

        if (mStartButton != null) {
            mStartButton.setOnClickListener(this);
            updateStartImage();
        }

        if (mFullscreenButton != null) {
            mFullscreenButton.setOnClickListener(this);
            mFullscreenButton.setOnTouchListener(this);
        }

        if (mProgressBar != null) {
//            mProgressBar.setOnSeekBarChangeListener(this);
        }

        if (mBottomContainer != null) {
            mBottomContainer.setOnClickListener(this);
        }

        if (mTextureViewContainer != null) {
            mTextureViewContainer.setOnClickListener(this);
            mTextureViewContainer.setOnTouchListener(this);
        }

        if (mProgressBar != null) {
            mProgressBar.setOnTouchListener(this);
        }

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
        prepare();
        start();
    }

    /**
     * 定义开始按键显示
     */
    protected void updateStartImage() {
        ImageView imageView = (ImageView) mStartButton;
//        if (mCurrentState == CURRENT_STATE_PLAYING) {
//            imageView.setImageResource(R.drawable.ic_pause);
//        } else if (mCurrentState == CURRENT_STATE_ERROR) {
//            imageView.setImageResource(R.drawable.ic_pause);
//        } else {
            imageView.setImageResource(R.drawable.ic_play);
//        }
    }

}

package com.songlei.xplayer.view;

import android.app.Dialog;
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.songlei.xplayer.R;
import com.songlei.xplayer.base.Option;
import com.songlei.xplayer.listener.PPPlayerViewListener;

/**
 * 标准播放器布局，根据该布局自定义，id不可变
 * Created by songlei on 2019/07/02.
 */
public class PPVideoPlayerView extends PPOrientationView {
    //切换缩放模式
    private TextView mMoreScale;
    //记住切换数据源类型
    private int mShowType = 0;
    private PPPlayerViewListener mPPPlayerViewListener;

    //===========播放进度dialog=======
    //触摸进度dialog
    protected Dialog mProgressDialog;
    //触摸进度条的progress
    protected ProgressBar mDialogProgressBar;
    //触摸移动显示文本
    protected TextView mDialogSeekTime;
    //触摸移动显示全部时间
    protected TextView mDialogTotalTime;
    //触摸移动方向icon
    protected ImageView mDialogIcon;
    protected Drawable mDialogProgressBarDrawable;
    protected int mDialogProgressHighLightColor = -11;
    protected int mDialogProgressNormalColor = -11;

    public PPVideoPlayerView(Context context) {
        super(context);
    }

    public PPVideoPlayerView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public PPVideoPlayerView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    @Override
    protected void init(Context context) {
        super.init(context);
        initView();
    }

    private void initView(){
        mMoreScale = findViewById(R.id.moreScale);

        mMoreScale.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mShowType == 0) {
                    mShowType = 1;
                } else if (mShowType == 1) {
                    mShowType = 2;
                } else if (mShowType == 2) {
                    mShowType = 3;
                } else if (mShowType == 3) {
                    mShowType = 4;
                } else if (mShowType == 4) {
                    mShowType = 0;
                }
                resolveTypeUI();
            }
        });

        mFullscreenButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mIfCurrentIsFullScreen) {
                    exitFullScreen();
                } else {
                    enterFullScreen();
                }
            }
        });

        mBackButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mPPPlayerViewListener != null) {
                    mPPPlayerViewListener.onClickBack();
                }
            }
        });
    }

    public void setUp(String url){
        setUrl(url);
    }

    @Override
    public int getLayoutId() {
        return R.layout.layout_player_base;
    }

    /**
     * 显示比例
     * 注意，GSYVideoType.setShowType是全局静态生效，除非重启APP。
     */
    private void resolveTypeUI() {
        if (mShowType == 1) {
            mMoreScale.setText("16:9");
            Option.setShowType(Option.SCREEN_TYPE_16_9);
        } else if (mShowType == 2) {
            mMoreScale.setText("4:3");
            Option.setShowType(Option.SCREEN_TYPE_4_3);
        } else if (mShowType == 3) {
            mMoreScale.setText("全屏");
            Option.setShowType(Option.SCREEN_TYPE_FULL);
        } else if (mShowType == 4) {
            mMoreScale.setText("拉伸全屏");
            Option.setShowType(Option.SCREEN_MATCH_FULL);
        } else if (mShowType == 0) {
            mMoreScale.setText("默认比例");
            Option.setShowType(Option.SCREEN_TYPE_DEFAULT);
        }
        changeViewShowType();
        if (mTextureView != null)
            mTextureView.requestLayout();
    }

    //显示横屏
    public void enterFullScreen() {
        if (mOrientationUtil.getIsLand() != 1) {
            //直接横屏
            mOrientationUtil.resolveByClick();
        }
        onEnterFullScreen(mContext, true, true);
    }

    //实现竖屏
    public void exitFullScreen(){
        if (mOrientationUtil != null) {
            mOrientationUtil.backToProtVideo();
        }
        onExitFullScreen();
    }

    public void setPPPlayerViewListener(PPPlayerViewListener mPPPlayerViewListener){
        this.mPPPlayerViewListener = mPPPlayerViewListener;
    }

    //===============================ControlView的抽象接口=======================================
    @Override
    protected void showFullScreen() {
        setViewShowState(mLockScreen, VISIBLE);
    }

    @Override
    protected void showVerticalScreen() {
        setViewShowState(mLockScreen, INVISIBLE);
    }

    @Override
    protected void hideAllWidget() {
        setViewShowState(mBottomContainer, INVISIBLE);
        setViewShowState(mTopContainer, INVISIBLE);
    }

    @Override
    protected void showAllWidget() {
        setViewShowState(mBottomContainer, VISIBLE);
        setViewShowState(mTopContainer, VISIBLE);
    }

    @Override
    protected void showProgressDialog(float deltaX, String seekTime, int seekTimePosition, String totalTime, int totalTimeDuration) {
        if (mProgressDialog == null) {
            View localView = LayoutInflater.from(getContext()).inflate(getProgressDialogLayoutId(), null);
            if (localView.findViewById(getProgressDialogProgressId()) instanceof ProgressBar) {
                mDialogProgressBar = ((ProgressBar) localView.findViewById(getProgressDialogProgressId()));
                if (mDialogProgressBarDrawable != null) {
                    mDialogProgressBar.setProgressDrawable(mDialogProgressBarDrawable);
                }
            }
            if (localView.findViewById(getProgressDialogCurrentDurationTextId()) instanceof TextView) {
                mDialogSeekTime = ((TextView) localView.findViewById(getProgressDialogCurrentDurationTextId()));
            }
            if (localView.findViewById(getProgressDialogAllDurationTextId()) instanceof TextView) {
                mDialogTotalTime = ((TextView) localView.findViewById(getProgressDialogAllDurationTextId()));
            }
            if (localView.findViewById(getProgressDialogImageId()) instanceof ImageView) {
                mDialogIcon = ((ImageView) localView.findViewById(getProgressDialogImageId()));
            }
            mProgressDialog = new Dialog(getContext(), R.style.video_style_dialog_progress);
            mProgressDialog.setContentView(localView);
            mProgressDialog.getWindow().addFlags(Window.FEATURE_ACTION_BAR);
            mProgressDialog.getWindow().addFlags(32);
            mProgressDialog.getWindow().addFlags(16);
            mProgressDialog.getWindow().setLayout(getWidth(), getHeight());
            if (mDialogProgressNormalColor != -11 && mDialogTotalTime != null) {
                mDialogTotalTime.setTextColor(mDialogProgressNormalColor);
            }
            if (mDialogProgressHighLightColor != -11 && mDialogSeekTime != null) {
                mDialogSeekTime.setTextColor(mDialogProgressHighLightColor);
            }
            WindowManager.LayoutParams localLayoutParams = mProgressDialog.getWindow().getAttributes();
            localLayoutParams.gravity = Gravity.TOP;
            localLayoutParams.width = getWidth();
            localLayoutParams.height = getHeight();
            int location[] = new int[2];
            getLocationOnScreen(location);
            localLayoutParams.x = location[0];
            localLayoutParams.y = location[1];
            mProgressDialog.getWindow().setAttributes(localLayoutParams);
        }
        if (!mProgressDialog.isShowing()) {
            mProgressDialog.show();
        }
        if (mDialogSeekTime != null) {
            mDialogSeekTime.setText(seekTime);
        }
        if (mDialogTotalTime != null) {
            mDialogTotalTime.setText(" / " + totalTime);
        }
        if (totalTimeDuration > 0)
            if (mDialogProgressBar != null) {
                mDialogProgressBar.setProgress(seekTimePosition * 100 / totalTimeDuration);
            }
        if (deltaX > 0) {
            if (mDialogIcon != null) {
                mDialogIcon.setBackgroundResource(R.drawable.video_forward_icon);
            }
        } else {
            if (mDialogIcon != null) {
                mDialogIcon.setBackgroundResource(R.drawable.video_backward_icon);
            }
        }
    }

    /**
     * 触摸进度dialog的layoutId
     * 继承后重写可返回自定义
     * 有自定义的实现逻辑可重载showProgressDialog方法
     */
    protected int getProgressDialogLayoutId() {
        return R.layout.video_progress_dialog;
    }

    /**
     * 触摸进度dialog的进度条id
     * 继承后重写可返回自定义，如果没有可返回空
     * 有自定义的实现逻辑可重载showProgressDialog方法
     */
    protected int getProgressDialogProgressId() {
        return R.id.duration_progressbar;
    }

    /**
     * 触摸进度dialog的当前时间文本
     * 继承后重写可返回自定义，如果没有可返回空
     * 有自定义的实现逻辑可重载showProgressDialog方法
     */
    protected int getProgressDialogCurrentDurationTextId() {
        return R.id.tv_current;
    }

    /**
     * 触摸进度dialog全部时间文本
     * 继承后重写可返回自定义，如果没有可返回空
     * 有自定义的实现逻辑可重载showProgressDialog方法
     */
    protected int getProgressDialogAllDurationTextId() {
        return R.id.tv_duration;
    }

    /**
     * 触摸进度dialog的图片id
     * 继承后重写可返回自定义，如果没有可返回空
     * 有自定义的实现逻辑可重载showProgressDialog方法
     */
    protected int getProgressDialogImageId() {
        return R.id.duration_image_tip;
    }

    @Override
    protected void dismissProgressDialog() {
        if (mProgressDialog != null) {
            mProgressDialog.dismiss();
            mProgressDialog = null;
        }
    }

    @Override
    protected void dismissVolumeDialog() {

    }

    @Override
    protected void dismissBrightnessDialog() {

    }
}

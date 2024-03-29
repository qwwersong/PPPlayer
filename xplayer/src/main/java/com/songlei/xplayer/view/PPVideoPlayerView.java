package com.songlei.xplayer.view;

import android.app.Dialog;
import android.content.Context;
import android.content.IntentFilter;
import android.graphics.drawable.Drawable;
import android.net.ConnectivityManager;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import com.songlei.xplayer.R;
import com.songlei.xplayer.base.Option;
import com.songlei.xplayer.bean.VideoModeBean;
import com.songlei.xplayer.listener.PPPlayerViewListener;
import com.songlei.xplayer.util.CommonUtil;
import com.songlei.xplayer.util.MediaUtil;
import com.songlei.xplayer.util.NetChangedReceiver;
import com.songlei.xplayer.view.widget.SwitchModeDialog;
import com.songlei.xplayer.view.widget.VideoCover;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * 标准播放器布局，根据该布局自定义，id不可变
 * Created by songlei on 2019/07/02.
 */
public class PPVideoPlayerView extends PPOrientationView {
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
    //=================音量================
    //音量dialog
    protected Dialog mVolumeDialog;
    //音量进度条的progress
    protected ProgressBar mDialogVolumeProgressBar;
    protected Drawable mVolumeProgressDrawable;
    protected ImageView volumeIcon;
    //=================亮度================
    //亮度dialog
    protected Dialog mBrightnessDialog;
    //=================预览================
    private MediaUtil mMediaUtil;
    //预览窗口
    private RelativeLayout mPreviewLayout;
    private ImageView mPreView;
    //是否打开滑动预览
    private boolean mOpenPreView = true;
    //=================其他功能控件================
    //切换分辨率
    public TextView mSwitchSize;
    //状态覆盖控件
    private VideoCover mVideoCover;

    private PPPlayerViewListener mPPPlayerViewListener;
    private NetChangedReceiver mNetChangedReceiver;
    private Map<Integer, VideoModeBean> modeMap = new HashMap<>();
    private int playMode;

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
//        mMediaUtil = MediaUtil.getInstance();
        initView();
        initListener();
//        registerNetReceiver();
    }

    private void registerNetReceiver(){
        mNetChangedReceiver = new NetChangedReceiver();
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction("android.net.conn.CONNECTIVITY_CHANGE");
        intentFilter.addAction("android.net.wifi.WIFI_STATE_CHANGED");
        intentFilter.addAction("android.net.wifi.STATE_CHANGE");
        mContext.registerReceiver(mNetChangedReceiver, intentFilter);
        mNetChangedReceiver.setOnNetChangeListener(new NetChangedReceiver.OnNetChangeListener() {
            @Override
            public void onNetChange(int type) {
                if (type == ConnectivityManager.TYPE_MOBILE) {
                    showNoWiFi();
                }
            }
        });
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        if (mNetChangedReceiver != null) {
            mContext.unregisterReceiver(mNetChangedReceiver);
        }
    }

    protected void initView(){
        mSwitchSize = findViewById(R.id.switchSize);
        mSwitchSize.setVisibility(GONE);

        mVideoCover = findViewById(R.id.video_cover);
        mPreviewLayout = findViewById(R.id.preview_layout);
        mPreView = findViewById(R.id.preview_image);
    }

    protected void initListener(){
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

        mSwitchSize.setOnClickListener(new OnClickListener(){
            @Override
            public void onClick(View v) {
                showSwitchDialog();
            }
        });

        mVideoCover.setOnCoverListener(new VideoCover.OnCoverListener() {
            @Override
            public void onKeepPlay() {
                mVideoCover.hide();
                clickStartIcon();
            }

            @Override
            public void onNoWiFiKeepPlay(boolean isChecked) {
                mVideoCover.hide();
                clickStartIcon();
                mTextureViewContainer.setEnabled(true);
            }
        });
    }

    public void setUp(String url){
        setUrl(url);
    }

    public boolean setUp(List<VideoModeBean> list){
        modeMap.clear();
        for (VideoModeBean videoBean : list) {
            modeMap.put(videoBean.video_type, videoBean);
        }
        if (list.size() == 0) {
            return false;
        }
        playMode = list.get(0).video_type;
        VideoModeBean modeBean = modeMap.get(playMode);
        if (modeBean == null) {
            return false;
        }
        mSwitchSize.setText(modeBean.show_name);

//        mMediaUtil.setSource(modeBean.url);
        setUp(modeBean.url);
        return true;
    }

    public void setTitle(String title){
        if (!TextUtils.isEmpty(title) && mTitleTextView != null) {
            mTitleTextView.setText(title);
        }
    }

    @Override
    public int getLayoutId() {
        return R.layout.layout_player_base;
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

    public void onPause(){
        clickStartIcon();
    }

    public void onResume(){
        clickStartIcon();
    }

    public void onRelease(){
        stopProgressTimer();
        stop();
        release();
    }

    @Override
    protected void onStateLayout(int state) {
        super.onStateLayout(state);
        if (state == STATE_PREPARE && !mUrl.contains("m3u8")) {
//            startDownFrame(mUrl);
        }
    }

    public void setPPPlayerViewListener(PPPlayerViewListener mPPPlayerViewListener){
        this.mPPPlayerViewListener = mPPPlayerViewListener;
    }

    //===============================ControlView的抽象接口=======================================


//    @Override
//    public void clickStartIcon() {
//        super.clickStartIcon();
//        if (mPPPlayerViewListener != null) {
//            mPPPlayerViewListener.onClickStart();
//        }
//    }

    @Override
    protected void showFullScreen() {
        setViewShowState(mSwitchSize, VISIBLE);
        setViewShowState(mLockScreen, VISIBLE);
    }

    @Override
    protected void showVerticalScreen() {
        setViewShowState(mSwitchSize, GONE);
        setViewShowState(mLockScreen, GONE);
    }

    @Override
    protected void hideAllWidget() {
        Log.e("xxx", "hideAllWidget");
        setViewShowState(mStartButton, INVISIBLE);
        setViewShowState(mBottomContainer, INVISIBLE);
        setViewShowState(mTopContainer, INVISIBLE);
    }

    @Override
    protected void showAllWidget() {
        Log.e("xxx", "showAllWidget");
        setViewShowState(mStartButton, VISIBLE);
        setViewShowState(mBottomContainer, VISIBLE);
        setViewShowState(mTopContainer, VISIBLE);
    }

    @Override
    protected void lockTouchLogic() {
        super.lockTouchLogic();
        if (mLockCurScreen) {
            mOrientationUtil.setEnable(false);
        } else {
            mOrientationUtil.setEnable(true);
        }
    }

    @Override
    protected void onClickUiToggle() {
        //TODO::区分状态、横竖屏、操作其他控制按钮的时候重置消失时间
        if (mIfCurrentIsFullScreen && mLockCurScreen) {
            setViewShowState(mLockScreen, VISIBLE);
            return;
        }
        if (mBottomContainer.getVisibility() == VISIBLE) {
            //隐藏
            setViewShowState(mStartButton, INVISIBLE);
            setViewShowState(mTopContainer, INVISIBLE);
            setViewShowState(mBottomContainer, INVISIBLE);
            setViewShowState(mLockScreen, GONE);
        } else {
            //显示
            setViewShowState(mStartButton, VISIBLE);
            setViewShowState(mTopContainer, VISIBLE);
            setViewShowState(mBottomContainer, VISIBLE);
            setViewShowState(mLockScreen, (mIfCurrentIsFullScreen) ? VISIBLE : GONE);
        }
    }

    @Override
    protected void showNetError(int errorCode) {
        mVideoCover.showNoNet(errorCode);
    }

    @Override
    protected void showWifiDialog() {
        showNoWiFi();
    }

    protected void showNoWiFi(){
        mVideoCover.showNoWiFiTip();
        setViewShowState(mBottomContainer, GONE);
        setViewShowState(mLockScreen, GONE);
        mTextureViewContainer.setEnabled(false);
        if (isPlaying()) {
            pause();
        }
    }

    @Override
    protected void showProgressDialog(float deltaX, String seekTime, int seekTimePosition, String totalTime, int totalTimeDuration) {
        if (mProgressDialog == null) {
            View localView = LayoutInflater.from(getContext()).inflate(getProgressDialogLayoutId(), null);
            if (localView.findViewById(getProgressDialogProgressId()) instanceof ProgressBar) {
                mDialogProgressBar = localView.findViewById(getProgressDialogProgressId());
                if (mDialogProgressBarDrawable != null) {
                    mDialogProgressBar.setProgressDrawable(mDialogProgressBarDrawable);
                }
            }
            if (localView.findViewById(getProgressDialogCurrentDurationTextId()) instanceof TextView) {
                mDialogSeekTime = localView.findViewById(getProgressDialogCurrentDurationTextId());
            }
            if (localView.findViewById(getProgressDialogAllDurationTextId()) instanceof TextView) {
                mDialogTotalTime = localView.findViewById(getProgressDialogAllDurationTextId());
            }
            if (localView.findViewById(getProgressDialogImageId()) instanceof ImageView) {
                mDialogIcon = localView.findViewById(getProgressDialogImageId());
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
                mDialogIcon.setBackgroundResource(R.drawable.ic_video_forward);
            }
        } else {
            if (mDialogIcon != null) {
                mDialogIcon.setBackgroundResource(R.drawable.ic_video_backward);
            }
        }
    }

    /**
     * 触摸进度dialog的layoutId
     * 继承后重写可返回自定义
     * 有自定义的实现逻辑可重载showProgressDialog方法
     */
    protected int getProgressDialogLayoutId() {
        return R.layout.dialog_video_progress;
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
    protected void showVolumeDialog(float deltaY, int volumePercent) {
        if (mVolumeDialog == null) {
            View localView = LayoutInflater.from(getContext()).inflate(getVolumeLayoutId(), null);

            if (mIfCurrentIsFullScreen){
                RelativeLayout container = localView.findViewById(R.id.volume_container);
                LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(container.getLayoutParams());
                lp.gravity = Gravity.TOP | Gravity.CENTER;
                lp.height = CommonUtil.dip2px(getContext(), 80);
                container.setLayoutParams(lp);
            }
            volumeIcon = localView.findViewById(R.id.volume_icon);
            if (localView.findViewById(getVolumeProgressId()) instanceof ProgressBar) {
                mDialogVolumeProgressBar = ((ProgressBar) localView.findViewById(getVolumeProgressId()));
                if (mVolumeProgressDrawable != null && mDialogVolumeProgressBar != null) {
                    mDialogVolumeProgressBar.setProgressDrawable(mVolumeProgressDrawable);
                }
            }
            mVolumeDialog = new Dialog(getContext(), R.style.video_style_dialog_progress);
            mVolumeDialog.setContentView(localView);
            mVolumeDialog.getWindow().addFlags(8);
            mVolumeDialog.getWindow().addFlags(32);
            mVolumeDialog.getWindow().addFlags(16);
            mVolumeDialog.getWindow().setLayout(-2, -2);
            mVolumeDialog.getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
            WindowManager.LayoutParams localLayoutParams = mVolumeDialog.getWindow().getAttributes();
            localLayoutParams.gravity = Gravity.TOP | Gravity.CENTER;
            localLayoutParams.width = getWidth();
            localLayoutParams.height = getHeight();
            int location[] = new int[2];
            getLocationOnScreen(location);
            localLayoutParams.x = location[0];
            localLayoutParams.y = location[1];
            mVolumeDialog.getWindow().setAttributes(localLayoutParams);
        }
        if (!mVolumeDialog.isShowing()) {
            mVolumeDialog.show();
        }
        if (mDialogVolumeProgressBar != null) {
            mDialogVolumeProgressBar.setProgress(volumePercent);
            if (volumePercent <= 0){
                volumeIcon.setImageResource(R.drawable.ic_mute);
            } else {
                volumeIcon.setImageResource(R.drawable.ic_sound);
            }
        }
    }

    @Override
    protected void dismissVolumeDialog() {
        if (mVolumeDialog != null) {
            mVolumeDialog.dismiss();
            mVolumeDialog = null;
        }
    }

    protected int getVolumeLayoutId() {
        return R.layout.dialog_video_volume;
    }

    /**
     * 音量dialog的百分比进度条 id
     * 继承后重写可返回自定义，如果没有可返回空
     * 有自定义的实现逻辑可重载showVolumeDialog方法
     */
    protected int getVolumeProgressId() {
        return R.id.volume_progressbar;
    }

    @Override
    protected void showBrightnessDialog(float percent) {
        if (mBrightnessDialog == null) {
            View localView = LayoutInflater.from(getContext()).inflate(getBrightnessLayoutId(), null);
            if (mIfCurrentIsFullScreen){
                RelativeLayout container = localView.findViewById(R.id.volume_container);
                LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(container.getLayoutParams());
                lp.gravity = Gravity.TOP | Gravity.CENTER;
                lp.height = CommonUtil.dip2px(getContext(), 80);
                container.setLayoutParams(lp);
            }
            volumeIcon = localView.findViewById(R.id.volume_icon);
            volumeIcon.setImageResource(R.drawable.ic_light);
            if (localView.findViewById(getVolumeProgressId()) instanceof ProgressBar) {
                mDialogVolumeProgressBar = ((ProgressBar) localView.findViewById(getVolumeProgressId()));
                if (mVolumeProgressDrawable != null && mDialogVolumeProgressBar != null) {
                    mDialogVolumeProgressBar.setProgressDrawable(mVolumeProgressDrawable);
                }
            }
            mBrightnessDialog = new Dialog(getContext(), R.style.video_style_dialog_progress);
            mBrightnessDialog.setContentView(localView);
            mBrightnessDialog.getWindow().addFlags(8);
            mBrightnessDialog.getWindow().addFlags(32);
            mBrightnessDialog.getWindow().addFlags(16);
            mBrightnessDialog.getWindow().setLayout(-2, -2);
            mBrightnessDialog.getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
            WindowManager.LayoutParams localLayoutParams = mBrightnessDialog.getWindow().getAttributes();
            localLayoutParams.gravity = Gravity.TOP | Gravity.CENTER;
            localLayoutParams.width = getWidth();
            localLayoutParams.height = getHeight();
            int location[] = new int[2];
            getLocationOnScreen(location);
            localLayoutParams.x = location[0];
            localLayoutParams.y = location[1];
            mBrightnessDialog.getWindow().setAttributes(localLayoutParams);
        }
        if (!mBrightnessDialog.isShowing()) {
            mBrightnessDialog.show();
        }
        if (mDialogVolumeProgressBar != null) {
            mDialogVolumeProgressBar.setProgress((int) (percent * 100));
        }
    }

    @Override
    protected void dismissBrightnessDialog() {
        if (mBrightnessDialog != null) {
            mBrightnessDialog.dismiss();
            mBrightnessDialog = null;
        }
    }

    /**
     * 亮度dialog的layoutId
     * 继承后重写可返回自定义
     * 有自定义的实现逻辑可重载showBrightnessDialog方法
     */
    protected int getBrightnessLayoutId() {
        return R.layout.dialog_video_volume;
    }

    /**
     * 弹出切换清晰度
     */
    private void showSwitchDialog() {
        if (!mHadPlay) {
            return;
        }

        SwitchModeDialog mSwitchModeDialog = new SwitchModeDialog(mContext, false, mSwitchSize, modeMap, playMode);
        mSwitchModeDialog.setOnSwitchModeListener(new SwitchModeDialog.OnSwitchModeListener() {
            @Override
            public void onSwitchMode(int mode) {
                playMode = mode;
                String url = "";
                switch (mode) {
                    case Option.TYPE_MODE_NORMAL:
                        url = modeMap.get(Option.TYPE_MODE_NORMAL).url;
                        break;
                    case Option.TYPE_MODE_SUPER_CLEAR:
                        url = modeMap.get(Option.TYPE_MODE_SUPER_CLEAR).url;
                        break;
                    case Option.TYPE_MODE_HIGH_CLEAR:
                        url = modeMap.get(Option.TYPE_MODE_HIGH_CLEAR).url;
                        break;
                }

                if ((mCurrentState == STATE_PLAYING || mCurrentState == STATE_PAUSE)) {
                    pause();
                    mSeekOnStart = getCurrentPosition();
                    setPlayPosition(mSeekOnStart);
                    onRelease();
                    initPlayer();
                    setUp(url);
                    prepare();
                }
            }
        });
        mSwitchModeDialog.show();
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        super.onProgressChanged(seekBar, progress, fromUser);
        if (fromUser && mOpenPreView && !mUrl.contains("m3u8")) {
            int width = seekBar.getWidth();
            int time = progress * getDuration() / 100;
            int offset = (int) (width - (getResources().getDimension(R.dimen.seek_bar_image) / 2)) / 100 * progress;
            showPreView(mUrl, time);
            RelativeLayout.LayoutParams layoutParams = (RelativeLayout.LayoutParams) mPreviewLayout.getLayoutParams();
            layoutParams.leftMargin = offset;
            //设置帧预览图的显示位置
            mPreviewLayout.setLayoutParams(layoutParams);
        }
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        super.onStartTrackingTouch(seekBar);
        if (mOpenPreView && !mUrl.contains("m3u8")) {
            mPreviewLayout.setVisibility(VISIBLE);
        }
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        super.onStopTrackingTouch(seekBar);
        if (mOpenPreView && !mUrl.contains("m3u8")) {
            mPreviewLayout.setVisibility(GONE);
        }
    }

    //显示预览
    private void showPreView(String url, long time) {
        int width = CommonUtil.dip2px(getContext(), 150);
        int height = CommonUtil.dip2px(getContext(), 100);
//        mPreView.setImageBitmap(mMediaUtil.decodeFrame(time));

//        Glide.with(getContext().getApplicationContext())
//                .setDefaultRequestOptions(
//                        new RequestOptions()
//                                //这里限制了只从缓存读取
//                                .onlyRetrieveFromCache(true)
//                                .frame(1000 * time)
//                                .override(width, height)
//                                .dontAnimate()
//                                .centerCrop())
//                .load(url)
//                .into(mPreView);
    }

    private void startDownFrame(String url) {
        for (int i = 1; i <= 100; i++) {
            int time = i * getDuration() / 100;
            int width = CommonUtil.dip2px(getContext(), 150);
            int height = CommonUtil.dip2px(getContext(), 100);
//            mPreView.setImageBitmap(mMediaUtil.decodeFrame(time));

//            Glide.with(getContext().getApplicationContext())
//                    .setDefaultRequestOptions(
//                            new RequestOptions()
//                                    .frame(1000 * time)
//                                    .override(width, height)
//                                    .centerCrop())
//                    .load(url).preload(width, height);
        }
    }

    @Override
    protected void cloneParams(PPControlView from, PPControlView to) {
        super.cloneParams(from, to);
        if (to.mProgressBar != null && from.mProgressBar != null) {
            to.mProgressBar.setProgress(from.mProgressBar.getProgress());
            to.mProgressBar.setSecondaryProgress(from.mProgressBar.getSecondaryProgress());
        }
        if (to.mTotalTimeTextView != null && from.mTotalTimeTextView != null) {
            to.mTotalTimeTextView.setText(from.mTotalTimeTextView.getText());
        }
        if (to.mCurrentTimeTextView != null && from.mCurrentTimeTextView != null) {
            to.mCurrentTimeTextView.setText(from.mCurrentTimeTextView.getText());
        }
    }
}

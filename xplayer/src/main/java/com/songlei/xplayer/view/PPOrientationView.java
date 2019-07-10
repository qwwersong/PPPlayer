package com.songlei.xplayer.view;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.util.AttributeSet;

import com.songlei.xplayer.util.OrientationUtil;
import com.songlei.xplayer.util.PlayerLayoutHelper;

/**
 * 处理横竖屏逻辑
 * Created by songlei on 2019/07/02.
 */
public abstract class PPOrientationView extends PPControlView {
    //旋转工具类
    protected OrientationUtil mOrientationUtil;
    //保存系统状态UI
    protected int mSystemUiVisibility;//TODO::如何显示
    protected PlayerLayoutHelper mLayoutHelper;
    //当前是否全屏
    protected boolean mIfCurrentIsFullScreen = false;

    public PPOrientationView(Context context) {
        super(context);
        initData(context);
    }

    public PPOrientationView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initData(context);
    }

    public PPOrientationView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        initData(context);
    }

    private void initData(Context context){
        mSystemUiVisibility = ((Activity) context).getWindow().getDecorView().getSystemUiVisibility();
        mLayoutHelper = new PlayerLayoutHelper(context);
        mLayoutHelper.setLayoutType(this, PlayerLayoutHelper.TYPE_BIG);
    }

    //退出全屏
    public void onBackFullScreen(){
        if (mIfCurrentIsFullScreen) {//全屏时
            mIfCurrentIsFullScreen = false;
            if (mOrientationUtil != null) {
                mOrientationUtil.setEnable(false);
                mOrientationUtil.releaseListener();
                mOrientationUtil = null;
            }
            mLayoutHelper.setLayoutType(this, PlayerLayoutHelper.TYPE_BIG);
            //TODO::回调退出全屏
            //UI显示
            if (mHideKey) {
                PlayerLayoutHelper.showNavKey(mContext, mSystemUiVisibility);
            }
            PlayerLayoutHelper.showSupportActionBar(mContext, true, true);
        } else {//非全屏时
            //退出界面
        }
    }

    //进入全屏
    public void onEnterFullScreen(Context context, boolean actionBar, boolean statusBar){
        mSystemUiVisibility = ((Activity) context).getWindow().getDecorView().getSystemUiVisibility();

        PlayerLayoutHelper.hideSupportActionBar(mContext, actionBar, statusBar);

        if (mHideKey) {
            PlayerLayoutHelper.hideNavKey(mContext);
        }
        mLayoutHelper.setLayoutType(this, PlayerLayoutHelper.TYPE_FULL);

        mIfCurrentIsFullScreen = true;
        mOrientationUtil = new OrientationUtil((Activity) context);
        mOrientationUtil.setEnable(true);
//        mOrientationUtil.setRotateWithSystem();

//        final boolean isVertical = isVerticalFullByVideoSize();
//        final boolean isLockLand = isLockLandByAutoFullSize();
//
//        if (!isVertical && isLockLand) {
//            mOrientationUtils.resolveByClick();
//        }

        //TODO::回调进入全屏
    }

    //旋转处理
    @Override
    protected void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if (newConfig.orientation == ActivityInfo.SCREEN_ORIENTATION_USER) {
            if (!mIfCurrentIsFullScreen) {
                onEnterFullScreen(mContext, true, true);//TODO::actionBar和statusBar两个值的设置
            }
        } else {
            //新版本isIfCurrentIsFullscreen的标志位内部提前设置了，所以不会和手动点击冲突
            if (mIfCurrentIsFullScreen) {
                onBackFullScreen();
            }
            if (mOrientationUtil != null) {
                mOrientationUtil.setEnable(true);
            }
        }
    }
}

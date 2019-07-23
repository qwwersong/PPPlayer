package com.songlei.xplayer.view;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.util.AttributeSet;
import android.util.Log;

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

    private void initData(Context context) {
        mSystemUiVisibility = ((Activity) context).getWindow().getDecorView().getSystemUiVisibility();
        mLayoutHelper = new PlayerLayoutHelper(context);
        mLayoutHelper.setLayoutType(this, PlayerLayoutHelper.TYPE_BIG);
        mOrientationUtil = new OrientationUtil((Activity) context);
    }

    //退出全屏
    public void onExitFullScreen() {
        mIfCurrentIsFullScreen = false;
        mLayoutHelper.setLayoutType(this, PlayerLayoutHelper.TYPE_BIG);
        if (mHideKey) {
            PlayerLayoutHelper.showNavKey(mContext, mSystemUiVisibility);
        }
        PlayerLayoutHelper.showSupportActionBar(mContext, true, true);

        showVerticalScreen();
    }

    //进入全屏
    public void onEnterFullScreen(Context context, boolean actionBar, boolean statusBar) {
        mIfCurrentIsFullScreen = true;
        mLayoutHelper.setLayoutType(this, PlayerLayoutHelper.TYPE_FULL);
        mSystemUiVisibility = ((Activity) context).getWindow().getDecorView().getSystemUiVisibility();
        if (mHideKey) {
            PlayerLayoutHelper.hideNavKey(mContext);
        }
        PlayerLayoutHelper.hideSupportActionBar(mContext, actionBar, statusBar);

        showFullScreen();
    }

    //旋转处理
    @Override
    protected void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if (newConfig.orientation == ActivityInfo.SCREEN_ORIENTATION_USER) {
            Log.e("xxx", "进入全屏 mIfCurrentIsFullScreen = " + mIfCurrentIsFullScreen);
            if (!mIfCurrentIsFullScreen) {
                onEnterFullScreen(mContext, true, true);//TODO::actionBar和statusBar两个值的设置
            }
        } else {
            Log.e("xxx", "退出全屏 mIfCurrentIsFullScreen = " + mIfCurrentIsFullScreen);
            if (mIfCurrentIsFullScreen) {
                onExitFullScreen();
            }
        }
    }

    protected abstract void showFullScreen();

    protected abstract void showVerticalScreen();
}

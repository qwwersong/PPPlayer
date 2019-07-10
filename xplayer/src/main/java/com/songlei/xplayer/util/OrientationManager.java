package com.songlei.xplayer.util;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.database.ContentObserver;
import android.os.Handler;
import android.provider.Settings;
import android.view.OrientationEventListener;

/**
 * 注册屏幕旋转广播，用于锁屏
 * Created by songlei on 2018/08/03.
 */
public class OrientationManager {
//    private Context context;
    private static OrientationManager INSTANCE;
    public boolean isClickedZoom = false;
    public boolean isOrientationPortrait = true;
    private boolean isAutoRotateOn = false;
    private boolean isRotate;
    private RotationObserver mRotationObserver;
    private OrientationEventListenerImpl mOrientationImpl;

    private OrientationManager(){

    }

    public static OrientationManager getInstance(){
        if(INSTANCE == null){
            synchronized (OrientationManager.class){
                if(INSTANCE == null){
                    INSTANCE = new OrientationManager();
                }
            }
        }
        return INSTANCE;
    }

    public void initRotateListener(Context context) {
        if (!isRotate) {
            isRotate = true;
            isAutoRotateOn = (Settings.System.getInt(context.getContentResolver(), Settings.System.ACCELEROMETER_ROTATION, 0) == 1);
            mRotationObserver = new RotationObserver(new Handler(), context);
            mRotationObserver.startObserver();
            mOrientationImpl = new OrientationEventListenerImpl(context);
            mOrientationImpl.enable();
        }
    }

    public void removeRotateListener() {
        if (isRotate) {
            isRotate = false;
            if (mOrientationImpl != null) {
                mOrientationImpl.onDetach();
                mOrientationImpl.disable();
                mOrientationImpl = null;
            }
            if (mRotationObserver != null) {
                mRotationObserver.stopObserver();
                mRotationObserver = null;
            }
        }
    }

    class OrientationEventListenerImpl extends OrientationEventListener {
        private Context context;

        public OrientationEventListenerImpl(Context context) {
            super(context);
            this.context = context;
        }

        @Override
        public void onOrientationChanged(int rotation) {
            if (!isAutoRotateOn) { //没开启自动旋转就不做操作
                return;
            }
            if (!isClickedZoom) { //重力感应 自动旋转
                if ((rotation >= 0 && rotation <= 20) || (rotation >= 340 && rotation <= 360)) { // 设置为竖屏
                    if (ActivityInfo.SCREEN_ORIENTATION_PORTRAIT != context.getResources().getConfiguration().orientation) {
                        ((Activity)context).setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
                    }
                } else if (rotation <= 295 && rotation >= 255) { // 设置为横屏
                    if (ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE != context.getResources().getConfiguration().orientation) {
                        ((Activity)context).setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
                    }
                } else if (rotation >= 70 && rotation <= 110) {   //设置为横屏（逆向）
                    if (ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE != context.getResources().getConfiguration()
                            .orientation) {
                        ((Activity)context).setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE);
                    }
                }
            } else { //手动点击
                if (!isOrientationPortrait) {  //当前为横屏
                    if (rotation < 340 && rotation >= 255) {
                        isClickedZoom = false;
                    }
                    if (rotation > 20 && rotation < 110) {
                        isClickedZoom = false;
                    }
                } else { //当前为竖屏
                    if (rotation >= 300) {
                        isClickedZoom = false;
                    }
                }
            }
        }

        public void onDetach(){
            context = null;
        }
    }

    //观察屏幕旋转设置变化，类似于注册动态广播监听变化机制
    private class RotationObserver extends ContentObserver {
        ContentResolver mResolver;

        public RotationObserver(Handler handler, Context context) {
            super(handler);
            mResolver = context.getContentResolver();
        }

        //屏幕旋转设置改变时调用
        @Override
        public void onChange(boolean selfChange) {
            super.onChange(selfChange);
            isAutoRotateOn = (Settings.System.getInt(mResolver, Settings.System.ACCELEROMETER_ROTATION, 0)
                    == 1);
        }

        public void startObserver() {
            mResolver.registerContentObserver(Settings.System.getUriFor(Settings.System.ACCELEROMETER_ROTATION),
                    false, this);
        }

        private void stopObserver() {
            mResolver.unregisterContentObserver(this);
        }
    }

    public void onDetach(){
        isClickedZoom = false;
        isOrientationPortrait = true;
        isAutoRotateOn = false;
        isRotate = false;
    }
}

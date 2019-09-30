package com.songlei.xplayer.util;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.WindowManager;
import android.widget.RelativeLayout;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.view.ContextThemeWrapper;
import androidx.fragment.app.FragmentActivity;


/**
 * 播放器宽高布局控制类
 * Created by songlei on 2018/12/07.
 */
public class PlayerLayoutHelper {
    public static final int TYPE_BIG = 1;
    public static final int TYPE_FULL = 2;
    private RelativeLayout.LayoutParams layoutParamsBig;
    private RelativeLayout.LayoutParams layoutParamsFull;

    public PlayerLayoutHelper(Context context){
        int width_big = getScreenWidth(context);
        int height_big = width_big * 9 / 16; //宽高比16:9
        layoutParamsBig = new RelativeLayout.LayoutParams(width_big, height_big);

        int width_full = getScreenHeight(context);
        int height_full = getScreenWidth(context);
        layoutParamsFull = new RelativeLayout.LayoutParams(width_full, height_full);
    }

    public void setLayoutType(View view, int type){
        switch (type) {
            case TYPE_BIG:
                view.setLayoutParams(layoutParamsBig);
                break;
            case TYPE_FULL:
                view.setLayoutParams(layoutParamsFull);
                break;
        }
    }

    private int getScreenWidth(Context context) {
        DisplayMetrics displayMetrics = context.getResources().getDisplayMetrics();
        return displayMetrics.widthPixels;
    }

    private int getScreenHeight(Context context) {
        DisplayMetrics displayMetrics = context.getResources().getDisplayMetrics();
        return displayMetrics.heightPixels;
    }

    private int dip2px(Context context,float dipValue) {
        final float scale = context.getResources().getDisplayMetrics().density;
        return (int) (dipValue * scale + 0.5f);
    }

    //显示虚拟按键
    public static void showNavKey(Context context, int systemUiVisibility) {
        ((Activity) context).getWindow().getDecorView().setSystemUiVisibility(systemUiVisibility);
    }

    //隐藏虚拟按键
    public static void hideNavKey(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            //       设置屏幕始终在前面，不然点击鼠标，重新出现虚拟按键
            ((Activity) context).getWindow().getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION // hide nav
                            // bar
                            | View.SYSTEM_UI_FLAG_IMMERSIVE);
        } else {
            ((Activity) context).getWindow().getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION // hide nav
            );
        }
    }

    //显示actionBar和statusBar
    @SuppressLint("RestrictedApi")
    public static void showSupportActionBar(Context context, boolean actionBar, boolean statusBar) {
        if (actionBar) {
            AppCompatActivity appCompatActivity = PlayerLayoutHelper.getAppCompActivity(context);
            if (appCompatActivity != null) {
                ActionBar ab = appCompatActivity.getSupportActionBar();
                if (ab != null) {
                    ab.setShowHideAnimationEnabled(false);
                    ab.show();
                }
            }
        }

        if (statusBar) {
            if (context instanceof FragmentActivity) {
                FragmentActivity fragmentActivity = (FragmentActivity) context;
                fragmentActivity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
            } else {
                PlayerLayoutHelper.getAppCompActivity(context).getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
            }
        }
    }

    //隐藏actionBar和statusBar
    @SuppressLint("RestrictedApi")
    public static void hideSupportActionBar(Context context, boolean actionBar, boolean statusBar) {
        if (actionBar) {
            AppCompatActivity appCompatActivity = PlayerLayoutHelper.getAppCompActivity(context);
            if (appCompatActivity != null) {
                ActionBar ab = appCompatActivity.getSupportActionBar();
                if (ab != null) {
                    ab.setShowHideAnimationEnabled(false);
                    ab.hide();
                }
            }
        }
        if (statusBar) {
            if (context instanceof FragmentActivity) {
                FragmentActivity fragmentActivity = (FragmentActivity) context;
                fragmentActivity.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                        WindowManager.LayoutParams.FLAG_FULLSCREEN);
            } else {
                PlayerLayoutHelper.getAppCompActivity(context).getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                        WindowManager.LayoutParams.FLAG_FULLSCREEN);
            }
        }
    }

    /**
     * Get AppCompatActivity from context
     *
     * @param context
     * @return AppCompatActivity if it's not null
     */
    public static AppCompatActivity getAppCompActivity(Context context) {
        if (context == null) return null;
        if (context instanceof AppCompatActivity) {
            return (AppCompatActivity) context;
        } else if (context instanceof ContextThemeWrapper) {
            return getAppCompActivity(((ContextThemeWrapper) context).getBaseContext());
        }
        return null;
    }
}

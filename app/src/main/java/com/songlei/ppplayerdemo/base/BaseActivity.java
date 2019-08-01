package com.songlei.ppplayerdemo.base;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;

import com.songlei.xplayer.view.PPVideoPlayerView;

/**
 * Created by songlei on 2019/07/30.
 */
public abstract class BaseActivity<T extends PPVideoPlayerView> extends AppCompatActivity {

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(getLayoutId());
        initData();
        initView();
        initListener();
    }

    @Override
    protected void onPause() {
        super.onPause();
        getVideoView().onPause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        getVideoView().onRelease();
    }

    @Override
    public void onBackPressed() {
        if (getVideoView().mIfCurrentIsFullScreen) {
            getVideoView().exitFullScreen();
        } else {
            super.onBackPressed();
        }
    }

    public abstract int getLayoutId();

    public abstract void initData();

    public abstract void initView();

    public abstract void initListener();

    public abstract T getVideoView();
}

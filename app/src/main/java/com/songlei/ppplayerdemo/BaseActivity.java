package com.songlei.ppplayerdemo;

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

    abstract int getLayoutId();

    abstract void initData();

    abstract void initView();

    abstract void initListener();

    abstract T getVideoView();
}

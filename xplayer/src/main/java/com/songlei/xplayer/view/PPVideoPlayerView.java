package com.songlei.xplayer.view;

import android.content.Context;
import android.util.AttributeSet;
import android.view.Surface;

import com.songlei.xplayer.R;

/**
 * 标准播放器布局，根据该布局自定义，id不可变
 * Created by songlei on 2019/07/02.
 */
public class PPVideoPlayerView extends PPOrientationView {

    public PPVideoPlayerView(Context context) {
        super(context);
    }

    public PPVideoPlayerView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public PPVideoPlayerView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public void setUp(String url){
        setUrl(url);
    }

    @Override
    protected void releaseSurface(Surface surface) {

    }

    @Override
    public int getLayoutId() {
        return R.layout.layout_player_base;
    }

}

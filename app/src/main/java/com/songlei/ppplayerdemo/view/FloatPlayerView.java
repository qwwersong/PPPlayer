package com.songlei.ppplayerdemo.view;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.FrameLayout;

/**
 * Created by songlei on 2019/08/01.
 */
public class FloatPlayerView extends FrameLayout {
    private FloatVideoView floatVideoView;

    public FloatPlayerView(@NonNull Context context) {
        super(context);
        init();
    }

    public FloatPlayerView(@NonNull Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public FloatPlayerView(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    private void init(){
        floatVideoView = new FloatVideoView(getContext());

        LayoutParams layoutParams = new LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        layoutParams.gravity = Gravity.CENTER;

        addView(floatVideoView, layoutParams);

        String url = "http://9890.vod.myqcloud.com/9890_4e292f9a3dd011e6b4078980237cc3d3.f20.mp4";

        floatVideoView.setUp(url);

    }

    public void onRelease(){
        floatVideoView.onRelease();
    }
}

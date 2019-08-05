package com.songlei.ppplayerdemo.view;

import android.content.Context;
import android.media.AudioManager;
import android.util.AttributeSet;
import android.view.View;

import com.songlei.ppplayerdemo.R;
import com.songlei.xplayer.view.PPVideoPlayerView;

/**
 * Created by songlei on 2019/08/01.
 */
public class FloatVideoView extends PPVideoPlayerView {


    public FloatVideoView(Context context) {
        super(context);
    }

    public FloatVideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void init(Context context) {
        if (getContext() != null) {
            mContext = getContext();
        } else {
            mContext = context;
        }

        initInflate(mContext);

        mScreenWidth = mContext.getResources().getDisplayMetrics().widthPixels;
        mScreenHeight = mContext.getResources().getDisplayMetrics().heightPixels;

        mAudioManager = (AudioManager) mContext.getApplicationContext().getSystemService(Context.AUDIO_SERVICE);
        mAudioManager.requestAudioFocus(onAudioFocusChangeListener, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);

        mTextureViewContainer = findViewById(R.id.surface_container);
        mStartButton = findViewById(R.id.start);

        mStartButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                clickStartIcon();
            }
        });
        updateStartImage();
    }

    @Override
    public int getLayoutId() {
        return R.layout.layout_float_video;
    }
}

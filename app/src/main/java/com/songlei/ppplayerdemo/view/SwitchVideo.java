package com.songlei.ppplayerdemo.view;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.TextView;

import com.songlei.ppplayerdemo.R;
import com.songlei.xplayer.view.PPVideoPlayerView;

/**
 * Created by songlei on 2019/08/02.
 */
public class SwitchVideo extends PPVideoPlayerView {
    private TextView detailBtn;
    private int playPosition;//列表播放的位置

    public SwitchVideo(Context context) {
        super(context);
    }

    public SwitchVideo(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void init(Context context) {
        super.init(context);
        detailBtn = findViewById(R.id.detail_btn);
        detailBtn.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {

            }
        });

        updateStartImage();
    }

    @Override
    public int getLayoutId() {
        return R.layout.switch_video;
    }

    public void setPlayPosition(int playPosition){
        this.playPosition = playPosition;
    }

    public int getPlayPosition(){
        return playPosition;
    }

}

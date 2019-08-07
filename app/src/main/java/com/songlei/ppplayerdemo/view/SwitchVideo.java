package com.songlei.ppplayerdemo.view;

import android.app.Activity;
import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.TextView;

import com.songlei.ppplayerdemo.R;
import com.songlei.ppplayerdemo.activity.SwitchDetailActivity;
import com.songlei.ppplayerdemo.util.SmallVideoHelper;
import com.songlei.ppplayerdemo.util.SwitchUtil;
import com.songlei.xplayer.view.PPVideoPlayerView;

/**
 * Created by songlei on 2019/08/02.
 */
public class SwitchVideo extends PPVideoPlayerView {
    private TextView detailBtn;
    private SmallVideoHelper smallVideoHelper;

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
                SwitchUtil.savePlayerState(SwitchVideo.this);

                SwitchDetailActivity.startDetailActivity((Activity) getContext(), SwitchVideo.this);
            }
        });
        if (mIfCurrentIsFullScreen) {
            detailBtn.setVisibility(GONE);
        }

        updateStartImage();
    }

    @Override
    public int getLayoutId() {
        return R.layout.switch_video;
    }

    public void setSmallVideoHelper(SmallVideoHelper smallVideoHelper){
        this.smallVideoHelper = smallVideoHelper;
    }

    public SwitchVideo saveState(){
        SwitchVideo switchVideo = new SwitchVideo(getContext());
        cloneParams(this, switchVideo);
        return switchVideo;
    }

    public void cloneState(SwitchVideo switchVideo){
        cloneParams(switchVideo, this);
    }

    public void setSurfaceToPlay(){
//        clickStartIcon();
        addTextureView();
    }

    @Override
    public void clickStartIcon() {
        super.clickStartIcon();
        smallVideoHelper.setVideoPlayer(SwitchVideo.this);
    }
}

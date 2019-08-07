package com.songlei.ppplayerdemo.util;

import android.content.Context;
import android.graphics.Point;
import android.util.Log;

import com.songlei.xplayer.view.PPVideoPlayerView;

/**
 * Created by songlei on 2019/08/07.
 */
public class SmallVideoHelper {
    private PPVideoPlayerView mVideoPlayer;
    private Context mContext;
    private boolean isSmall;

    public SmallVideoHelper(Context context){
        mContext = context;
    }

    public void setVideoPlayer(PPVideoPlayerView playerView){
        Log.e("xxx", "SmallVideoHelper setVideoPlayer = " + playerView);
        mVideoPlayer = playerView;
    }

    public void showSmallVideo(Point size, boolean actionBar, boolean statusBar){
        isSmall = true;
        mVideoPlayer.showSmallVideo(size, actionBar, statusBar);
    }

    public void hideSmallVideo(){
        isSmall = false;
        mVideoPlayer.hideSmallVideo();
    }

    public boolean isSmall() {
        return isSmall;
    }

}

package com.songlei.ppplayerdemo.util;

import android.view.View;

import com.songlei.ppplayerdemo.view.SwitchVideo;

/**
 * Created by songlei on 2019/08/02.
 */
public class SwitchUtil {

    public static void optionPlayer(SwitchVideo switchVideo, String url, String title){
        switchVideo.getTitleTextView().setVisibility(View.GONE);
        switchVideo.getBackButton().setVisibility(View.GONE);
        switchVideo.setIsTouchWidget(false);
        switchVideo.setTitle(title);
        switchVideo.setUp(url);
    }

}

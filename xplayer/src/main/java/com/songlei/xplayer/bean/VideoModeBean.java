package com.songlei.xplayer.bean;

/**
 * Created by songlei on 2019/07/23.
 */
public class VideoModeBean {
    public String url;
    public String show_name;
    public int video_type;

    public VideoModeBean(String url, String show_name, int video_type) {
        this.url = url;
        this.show_name = show_name;
        this.video_type = video_type;
    }
}

package com.songlei.ppplayerdemo;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import com.songlei.xplayer.view.PPVideoPlayerView;

public class MainActivity extends AppCompatActivity {
    private PPVideoPlayerView pp_video_view;
//    private String url = "http://feichitest.yeepo.cn/video/58dd8559576d7093c48136ba55fa88b3/58dd8559576d7093c48136ba55fa88b3.vdo";
    private String url = "http://qvw.facebac.com/hls/16fe72d730196a7ce310/f76jl1oek1p947odyswl/n_n/db09c93d9c5386395f2086165e58f2ab.m3u8?xstToken=cf77715d";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        pp_video_view = findViewById(R.id.pp_video_view);
        pp_video_view.setUp(url);
    }
}

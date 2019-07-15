package com.songlei.ppplayerdemo;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;

import com.songlei.xplayer.listener.PPPlayerViewListener;
import com.songlei.xplayer.view.PPVideoPlayerView;

/**
 * Created by songlei on 2019/07/12.
 */
public class PlayerActivity extends AppCompatActivity {

    private PPVideoPlayerView pp_video_view;
    //    private String url = "http://feichitest.yeepo.cn/video/58dd8559576d7093c48136ba55fa88b3/58dd8559576d7093c48136ba55fa88b3.vdo";
    private String url = "http://qvw.facebac.com/hls/16fe72d730196a7ce310/f76jl1oek1p947odyswl/n_n/db09c93d9c5386395f2086165e58f2ab.m3u8?xstToken=cf77715d";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_player);
        pp_video_view = findViewById(R.id.pp_video_view);
        pp_video_view.setUp(url);

        pp_video_view.setPPPlayerViewListener(new PPPlayerViewListener() {
            @Override
            public void onClickBack() {
                onBackPressed();
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();
//        pp_video_view.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
        pp_video_view.onPause();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        pp_video_view.stop();
        pp_video_view.release();
    }

    @Override
    public void onBackPressed() {
        if (pp_video_view.mIfCurrentIsFullScreen) {
            pp_video_view.exitFullScreen();
        } else {
            //退出activity
            super.onBackPressed();
        }
    }

}

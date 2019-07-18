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
    private String url = "http://qvw.mvaas.cn/hls/2f03f5b30ec2f83c2f85/gs7ttsp7wbl6yukwrwbz/n_n/6414bf9903cca1299eecd7dd9bff0b94.m3u8?xstToken=888c0dc7";

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

package com.songlei.ppplayerdemo;

import android.annotation.TargetApi;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.view.ViewCompat;
import android.support.v7.app.AppCompatActivity;
import android.transition.Transition;

import com.songlei.ppplayerdemo.listener.OnTransitionListener;
import com.songlei.xplayer.bean.VideoModeBean;
import com.songlei.xplayer.listener.PPPlayerViewListener;
import com.songlei.xplayer.view.PPVideoPlayerView;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by songlei on 2019/07/12.
 */
public class PlayerActivity extends AppCompatActivity {
    public final static String IMG_TRANSITION = "IMG_TRANSITION";
    public final static String TRANSITION = "TRANSITION";

    /**
     * "urls": [{
     * 			"type": 0,
     * 			"typeName": "原始",
     * 			"url": "http://qvw.mvaas.cn/hls/2f03f5b30ec2f83c2f85/gs7ttsp7wbl6yukwrwbz/n_n/6414bf9903cca1299eecd7dd9bff0b94.m3u8?xstToken=888c0dc7"
     *                }, {
     * 			"type": 1,
     * 			"typeName": "标清",
     * 			"url": "http://qvw.mvaas.cn/hls/2f03f5b30ec2f83c2f85/gs7ttsp7wbl6yukwrwbz/480_25_4_500k_4_3_n/06e6ab5fdf1bd9ce4f8947e6774002f9.m3u8?xstToken=888c0dc7"
     *        }, {
     * 			"type": 2,
     * 			"typeName": "高清",
     * 			"url": "http://qvw.mvaas.cn/hls/2f03f5b30ec2f83c2f85/gs7ttsp7wbl6yukwrwbz/720_25_4_1000k_4_7_n/fa5212ef3b6da8de24bbcd845a669ba3.m3u8?xstToken=888c0dc7"
     *        }],
     */

    private boolean isTransition;
    private Transition transition;

    private PPVideoPlayerView pp_video_view;

    //    private String url = "http://feichitest.yeepo.cn/video/58dd8559576d7093c48136ba55fa88b3/58dd8559576d7093c48136ba55fa88b3.vdo";
    private String url = "http://qvw.mvaas.cn/hls/2f03f5b30ec2f83c2f85/gs7ttsp7wbl6yukwrwbz/n_n/6414bf9903cca1299eecd7dd9bff0b94.m3u8?xstToken=888c0dc7";
    private String b_url = "http://qvw.mvaas.cn/hls/2f03f5b30ec2f83c2f85/gs7ttsp7wbl6yukwrwbz/480_25_4_500k_4_3_n/06e6ab5fdf1bd9ce4f8947e6774002f9.m3u8?xstToken=888c0dc7";
    private String h_url = "http://qvw.mvaas.cn/hls/2f03f5b30ec2f83c2f85/gs7ttsp7wbl6yukwrwbz/720_25_4_1000k_4_7_n/fa5212ef3b6da8de24bbcd845a669ba3.m3u8?xstToken=888c0dc7";

    private List<VideoModeBean> urlList = new ArrayList<>();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_player);
        isTransition = getIntent().getBooleanExtra(TRANSITION, false);
        initData();
        initView();
        initListener();
        initTransition();
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

    private void initData(){
        urlList.add(new VideoModeBean(url, "原始", 0));
        urlList.add(new VideoModeBean(b_url, "标清", 1));
        urlList.add(new VideoModeBean(h_url, "高清", 2));
    }

    private void initView(){
        pp_video_view = findViewById(R.id.pp_video_view);

        pp_video_view.setUp(urlList);
    }

    private void initListener(){
        pp_video_view.setPPPlayerViewListener(new PPPlayerViewListener() {
            @Override
            public void onClickBack() {
                onBackPressed();
            }
        });
    }

    private void initTransition() {
        if (isTransition && Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            postponeEnterTransition();
            ViewCompat.setTransitionName(pp_video_view, IMG_TRANSITION);
            addTransitionListener();
            startPostponedEnterTransition();
        }
//        else {
//            pp_video_view.startPlayLogic();
//        }
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    private boolean addTransitionListener() {
        transition = getWindow().getSharedElementEnterTransition();
        if (transition != null) {
            transition.addListener(new OnTransitionListener(){
                @Override
                public void onTransitionEnd(Transition transition) {
                    super.onTransitionEnd(transition);
//                    videoPlayer.startPlayLogic();
                    transition.removeListener(this);
                }
            });
            return true;
        }
        return false;
    }

}

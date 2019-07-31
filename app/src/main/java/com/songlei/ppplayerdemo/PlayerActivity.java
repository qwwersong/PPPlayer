package com.songlei.ppplayerdemo;

import android.annotation.TargetApi;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Build;
import android.support.v4.view.ViewCompat;
import android.transition.Transition;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import com.songlei.ppplayerdemo.listener.OnTransitionListener;
import com.songlei.xplayer.base.Option;
import com.songlei.xplayer.bean.VideoModeBean;
import com.songlei.xplayer.listener.PPPlayerViewListener;
import com.songlei.xplayer.view.PPVideoPlayerView;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by songlei on 2019/07/12.
 */
public class PlayerActivity extends BaseActivity<PPVideoPlayerView> {
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

    private boolean decode_type = false;
    private int render_type = Constants.RENDER_TEXTURE;
    private int player_type = Constants.PLAYER_IJK;
    private float speed = 1;

    private PPVideoPlayerView pp_video_view;
    private Button bt_speed;

    //    private String url = "http://feichitest.yeepo.cn/video/58dd8559576d7093c48136ba55fa88b3/58dd8559576d7093c48136ba55fa88b3.vdo";
    private String m4_url = "http://9890.vod.myqcloud.com/9890_4e292f9a3dd011e6b4078980237cc3d3.f20.mp4";
    private String url = "http://qvw.mvaas.cn/hls/2f03f5b30ec2f83c2f85/gs7ttsp7wbl6yukwrwbz/n_n/6414bf9903cca1299eecd7dd9bff0b94.m3u8?xstToken=888c0dc7";
    private String b_url = "http://qvw.mvaas.cn/hls/2f03f5b30ec2f83c2f85/gs7ttsp7wbl6yukwrwbz/480_25_4_500k_4_3_n/06e6ab5fdf1bd9ce4f8947e6774002f9.m3u8?xstToken=888c0dc7";
    private String h_url = "http://qvw.mvaas.cn/hls/2f03f5b30ec2f83c2f85/gs7ttsp7wbl6yukwrwbz/720_25_4_1000k_4_7_n/fa5212ef3b6da8de24bbcd845a669ba3.m3u8?xstToken=888c0dc7";

    private List<VideoModeBean> urlList = new ArrayList<>();

    @Override
    int getLayoutId() {
        return R.layout.activity_player;
    }

    @Override
    void initData() {
        isTransition = getIntent().getBooleanExtra(Constants.TRANSITION, false);
        decode_type = getIntent().getBooleanExtra(Constants.DECODE_TYPE, false);
        render_type = getIntent().getIntExtra(Constants.RENDER_TYPE, Constants.RENDER_TEXTURE);
        player_type = getIntent().getIntExtra(Constants.PLAYER_TYPE, Constants.PLAYER_IJK);
        urlList.add(new VideoModeBean(m4_url, "原始", 0));
        urlList.add(new VideoModeBean(b_url, "标清", 1));
        urlList.add(new VideoModeBean(h_url, "高清", 2));

        Log.e("xxx", "onCreate decode_type = " + decode_type);
        Option.setPlayerMediaCodec(decode_type);
    }

    @Override
    void initView() {
        pp_video_view = findViewById(R.id.pp_video_view);
        bt_speed = findViewById(R.id.bt_speed);
        initVideo();
        initTransition();
    }

    @Override
    void initListener() {
        pp_video_view.setPPPlayerViewListener(new PPPlayerViewListener() {
            @Override
            public void onClickBack() {
                onBackPressed();
            }
        });
        bt_speed.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                resolveTypeUI();
            }
        });
    }

    private void initVideo(){
        Bitmap bitmap = BitmapFactory.decodeResource(getResources(), R.mipmap.ic_launcher);
        new Option.Builder()
                .setTitle("鸡你太美")
                .setUrlList(urlList)
                .setRenderType(render_type)
                .setPlayerType(player_type)
                .setMediaCodec(decode_type)
                .setWaterMarkBitmap(bitmap)
                .build(pp_video_view);
    }

    @Override
    PPVideoPlayerView getVideoView() {
        return pp_video_view;
    }

    private void resolveTypeUI() {
        if (speed == 1) {
            speed = 1.5f;
        } else if (speed == 1.5f) {
            speed = 2f;
        } else if (speed == 2) {
            speed = 0.5f;
        } else if (speed == 0.5f) {
            speed = 0.25f;
        } else if (speed == 0.25f) {
            speed = 1;
        }
        bt_speed.setText("播放速度：" + speed);
        pp_video_view.setSpeedPlaying(speed, true);
    }

    private void initTransition() {
        if (isTransition && Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            postponeEnterTransition();
            ViewCompat.setTransitionName(pp_video_view, Constants.IMG_TRANSITION);
            addTransitionListener();
            startPostponedEnterTransition();
        }
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    private boolean addTransitionListener() {
        transition = getWindow().getSharedElementEnterTransition();
        if (transition != null) {
            transition.addListener(new OnTransitionListener(){
                @Override
                public void onTransitionEnd(Transition transition) {
                    super.onTransitionEnd(transition);
                    transition.removeListener(this);
                }
            });
            return true;
        }
        return false;
    }

}

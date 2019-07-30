package com.songlei.ppplayerdemo;

import android.annotation.TargetApi;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Build;
import android.support.v4.view.ViewCompat;
import android.transition.Transition;
import android.util.Log;

import com.songlei.ppplayerdemo.listener.OnTransitionListener;
import com.songlei.xplayer.base.Option;
import com.songlei.xplayer.bean.VideoModeBean;
import com.songlei.xplayer.listener.PPPlayerViewListener;
import com.songlei.xplayer.view.PPRenderView;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by songlei on 2019/07/30.
 */
public class RenderActivity extends BaseActivity<PPRenderView> {
    private boolean isTransition;
    private Transition transition;

    private PPRenderView renderView;

    private boolean decode_type = false;
    private int render_type = Constants.RENDER_TEXTURE;
    private int player_type = Constants.PLAYER_IJK;

    private String m4_url = "http://9890.vod.myqcloud.com/9890_4e292f9a3dd011e6b4078980237cc3d3.f20.mp4";
    private String url = "http://qvw.mvaas.cn/hls/2f03f5b30ec2f83c2f85/gs7ttsp7wbl6yukwrwbz/n_n/6414bf9903cca1299eecd7dd9bff0b94.m3u8?xstToken=888c0dc7";
    private String b_url = "http://qvw.mvaas.cn/hls/2f03f5b30ec2f83c2f85/gs7ttsp7wbl6yukwrwbz/480_25_4_500k_4_3_n/06e6ab5fdf1bd9ce4f8947e6774002f9.m3u8?xstToken=888c0dc7";
    private String h_url = "http://qvw.mvaas.cn/hls/2f03f5b30ec2f83c2f85/gs7ttsp7wbl6yukwrwbz/720_25_4_1000k_4_7_n/fa5212ef3b6da8de24bbcd845a669ba3.m3u8?xstToken=888c0dc7";

    private List<VideoModeBean> urlList = new ArrayList<>();

    @Override
    int getLayoutId() {
        return R.layout.activity_render;
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
        renderView = findViewById(R.id.pp_video_view);
        initVideo();
        initTransition();
    }

    @Override
    void initListener() {
        renderView.setPPPlayerViewListener(new PPPlayerViewListener() {
            @Override
            public void onClickBack() {
                onBackPressed();
            }
        });
    }

    @Override
    PPRenderView getVideoView() {
        return renderView;
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
                .build(renderView);
    }

    private void initTransition() {
        if (isTransition && Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            postponeEnterTransition();
            ViewCompat.setTransitionName(renderView, Constants.IMG_TRANSITION);
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

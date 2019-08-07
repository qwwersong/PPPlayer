package com.songlei.ppplayerdemo.activity;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.ActivityCompat;
import android.support.v4.app.ActivityOptionsCompat;
import android.support.v4.view.ViewCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;

import com.songlei.ppplayerdemo.R;
import com.songlei.ppplayerdemo.util.SwitchUtil;
import com.songlei.ppplayerdemo.view.SwitchVideo;
import com.songlei.xplayer.listener.PPPlayerViewListener;

/**
 * Created by songlei on 2019/08/05.
 */
public class SwitchDetailActivity extends AppCompatActivity {

    private static final String OPTION_VIEW = "view";
    public static final String URL = "url";

    private SwitchVideo detailPlayer;

    public static void startDetailActivity(Activity activity, View transitionView){
        Intent intent = new Intent(activity, SwitchDetailActivity.class);
        // 这里制定了共享的视图元素
        ActivityOptionsCompat optionsCompat = ActivityOptionsCompat.makeSceneTransitionAnimation(activity, transitionView, OPTION_VIEW);
        ActivityCompat.startActivity(activity, intent, optionsCompat.toBundle());
    }

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_switch_video_detail);

        detailPlayer = findViewById(R.id.detail_player);

        String url = getIntent().getStringExtra(URL);
        Log.e("xxx", "SwitchDetailActivity url = " + url);
//        SwitchUtil.optionPlayer(detailPlayer, url, "这是坤");
        detailPlayer.setIsTouchWidget(true);
        SwitchUtil.clonePlayerState(detailPlayer);

        detailPlayer.setPPPlayerViewListener(new PPPlayerViewListener() {
            @Override
            public void onClickBack() {
                onBackPressed();
            }
        });

        detailPlayer.setSurfaceToPlay();

        ViewCompat.setTransitionName(detailPlayer, OPTION_VIEW);
    }

    @Override
    public void onBackPressed() {
        if (detailPlayer.mIfCurrentIsFullScreen) {
            detailPlayer.exitFullScreen();
        } else {
            super.onBackPressed();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        detailPlayer.onRelease();
        SwitchUtil.release();
    }
}

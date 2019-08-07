package com.songlei.ppplayerdemo.activity;

import android.graphics.Point;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.transition.Explode;
import android.view.Window;
import android.widget.AbsListView;
import android.widget.ListView;

import com.songlei.ppplayerdemo.R;
import com.songlei.ppplayerdemo.adapter.SwitchListVideoAdapter;
import com.songlei.ppplayerdemo.util.SmallVideoHelper;
import com.songlei.xplayer.PlayerManager;
import com.songlei.xplayer.util.CommonUtil;

/**
 * Created by songlei on 2019/08/02.
 */
public class SwitchListVideoActivity extends AppCompatActivity {
    private ListView videoList;
    private SwitchListVideoAdapter listVideoAdapter;
    private SmallVideoHelper smallVideoHelper;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        // 设置一个exit transition
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            getWindow().requestFeature(Window.FEATURE_CONTENT_TRANSITIONS);
            getWindow().setEnterTransition(new Explode());
            getWindow().setExitTransition(new Explode());
        }
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_list_video);

        videoList = findViewById(R.id.video_list);

        smallVideoHelper = new SmallVideoHelper(this);

        listVideoAdapter = new SwitchListVideoAdapter(this, smallVideoHelper);
        videoList.setAdapter(listVideoAdapter);

        videoList.setOnScrollListener(new AbsListView.OnScrollListener() {
            @Override
            public void onScrollStateChanged(AbsListView view, int scrollState) {

            }

            @Override
            public void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount, int totalItemCount) {
                int lastVisibleItem = firstVisibleItem + visibleItemCount;

                int playerPosition = PlayerManager.getInstance().getPlayerPosition();
//                Log.e("xxx", "onScroll mPlayerPosition = " + playerPosition + " firstVisibleItem = "
//                        + firstVisibleItem + " lastVisibleItem = " + lastVisibleItem);
                if (playerPosition >= 0) {
                    //不可视的时候
                    if ((playerPosition < firstVisibleItem ||playerPosition > lastVisibleItem)) {
                        if (!smallVideoHelper.isSmall()) {
                            //显示小窗口
                            int size = CommonUtil.dip2px(SwitchListVideoActivity.this, 150);
                            smallVideoHelper.showSmallVideo(new Point(size, size), false, true);
                        }
//                        release();
//                        listVideoAdapter.notifyDataSetChanged();
                    } else {
                        if (smallVideoHelper.isSmall()) {
                            smallVideoHelper.hideSmallVideo();
                        }
                    }
                }
            }
        });
    }

    @Override
    public void onBackPressed() {
        super.onBackPressed();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        release();
    }

    private void release(){
        PlayerManager.getInstance().stop();
        PlayerManager.getInstance().release();
    }
}

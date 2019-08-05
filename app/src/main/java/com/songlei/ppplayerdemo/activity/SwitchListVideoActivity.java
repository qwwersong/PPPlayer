package com.songlei.ppplayerdemo.activity;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.widget.AbsListView;
import android.widget.ListView;

import com.songlei.ppplayerdemo.R;
import com.songlei.ppplayerdemo.adapter.SwitchListVideoAdapter;
import com.songlei.xplayer.PlayerManager;

/**
 * Created by songlei on 2019/08/02.
 */
public class SwitchListVideoActivity extends AppCompatActivity {
    private ListView videoList;
    private SwitchListVideoAdapter listVideoAdapter;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_list_video);

        videoList = findViewById(R.id.video_list);

        listVideoAdapter = new SwitchListVideoAdapter(this);
        videoList.setAdapter(listVideoAdapter);

        videoList.setOnScrollListener(new AbsListView.OnScrollListener() {
            @Override
            public void onScrollStateChanged(AbsListView view, int scrollState) {

            }

            @Override
            public void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount, int totalItemCount) {
                int lastVisibleItem = firstVisibleItem + visibleItemCount;

                int playerPosition = PlayerManager.getInstance().getPlayerPosition();
                Log.e("xxx", "onScroll playerPosition = " + playerPosition + " firstVisibleItem = "
                        + firstVisibleItem + " lastVisibleItem = " + lastVisibleItem);
                if (playerPosition >= 0) {
                    if ((playerPosition < firstVisibleItem ||playerPosition > lastVisibleItem)) {
                        release();
                        listVideoAdapter.notifyDataSetChanged();
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

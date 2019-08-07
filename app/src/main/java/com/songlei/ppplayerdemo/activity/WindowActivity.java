package com.songlei.ppplayerdemo.activity;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.support.annotation.Nullable;
import android.support.annotation.RequiresApi;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.view.animation.BounceInterpolator;
import android.widget.Button;

import com.songlei.ppplayerdemo.R;
import com.songlei.ppplayerdemo.view.FloatPlayerView;
import com.yhao.floatwindow.FloatWindow;
import com.yhao.floatwindow.MoveType;
import com.yhao.floatwindow.Screen;

/**
 * Created by songlei on 2019/08/01.
 */
public class WindowActivity extends AppCompatActivity {
    private Button btStartWindow;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_window);
        if (Build.VERSION.SDK_INT >= 23) {
            if (!hasPermission(this)) {
                requestAlertWindowPermission();
            }
        }

        btStartWindow = findViewById(R.id.start_window);
        btStartWindow.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                showWindowView();
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        if (FloatWindow.get() != null && FloatWindow.get().getView() instanceof FloatPlayerView) {
            ((FloatPlayerView)FloatWindow.get().getView()).onRelease();
        }
        FloatWindow.destroy();
    }

    @RequiresApi(api = 23)
    private void requestAlertWindowPermission() {
        Intent intent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:" + getPackageName()));
        startActivityForResult(intent, 1);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (Build.VERSION.SDK_INT >= 23){
            //todo 用23以上编译即可出现canDrawOverlays
            if (hasPermission(this)) {

            } else {
                this.finish();
            }
        }
    }

    private void showWindowView(){
        if (FloatWindow.get() != null) {
            return;
        }
        FloatPlayerView floatPlayerView = new FloatPlayerView(this);
        FloatWindow
                .with(getApplicationContext())
                .setView(floatPlayerView)
                .setWidth(Screen.width, 0.4f)
                .setHeight(Screen.width, 0.4f)
                .setX(Screen.width, 0.6f)
                .setY(Screen.height, 0.3f)
                .setMoveType(MoveType.slide)//可拖动悬浮窗
                .setMoveStyle(500, new BounceInterpolator())//贴边动画
                .setFilter(false)
                .setDesktopShow(true)
                .build();
        FloatWindow.get().show();
    }

    @RequiresApi(api = Build.VERSION_CODES.M)
    public static boolean hasPermission(Context context) {
        return Settings.canDrawOverlays(context);
    }
}

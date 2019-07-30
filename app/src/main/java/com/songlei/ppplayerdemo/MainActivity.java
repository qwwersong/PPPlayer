package com.songlei.ppplayerdemo;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.app.ActivityOptionsCompat;
import android.support.v4.util.Pair;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.RadioGroup;

public class MainActivity extends AppCompatActivity implements View.OnClickListener,
        RadioGroup.OnCheckedChangeListener {
    private Button bt_normal;
    private Button bt_render;

    private static boolean decode_type = false;
    private static int render_type = Constants.RENDER_TEXTURE;
    private static int player_type = Constants.PLAYER_IJK;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        bt_normal = findViewById(R.id.bt_normal);
        bt_render = findViewById(R.id.bt_render);

        RadioGroup rg_decode = findViewById(R.id.rg_decode);
        RadioGroup rg_render = findViewById(R.id.rg_render);
        RadioGroup rg_player_type = findViewById(R.id.rg_player_type);

        bt_normal.setOnClickListener(this);
        bt_render.setOnClickListener(this);

        rg_decode.setOnCheckedChangeListener(this);
        rg_render.setOnCheckedChangeListener(this);
        rg_player_type.setOnCheckedChangeListener(this);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.bt_normal:
                goToVideoPlayer(MainActivity.this, bt_normal, PlayerActivity.class);
                break;
            case R.id.bt_render:
                goToVideoPlayer(MainActivity.this, bt_render, RenderActivity.class);
                break;
        }
    }

    /**
     * 跳转到视频播放
     */
    public static void goToVideoPlayer(Activity activity, View view, Class<?> cls) {
        Intent intent = new Intent(activity, cls);
        intent.putExtra(Constants.TRANSITION, true);
        intent.putExtra(Constants.DECODE_TYPE, decode_type);
        intent.putExtra(Constants.RENDER_TYPE, render_type);
        intent.putExtra(Constants.PLAYER_TYPE, player_type);
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
            Pair pair = new Pair<>(view, Constants.IMG_TRANSITION);
            ActivityOptionsCompat activityOptions = ActivityOptionsCompat.makeSceneTransitionAnimation(
                    activity, pair);
            ActivityCompat.startActivity(activity, intent, activityOptions.toBundle());
        } else {
            activity.startActivity(intent);
            activity.overridePendingTransition(R.anim.abc_fade_in, R.anim.abc_fade_out);
        }
    }

    @Override
    public void onCheckedChanged(RadioGroup group, int checkedId) {
        switch (checkedId) {
            case R.id.rg_hard_decode:
                Log.e("xxx", "硬解码");
                decode_type = true;
                break;
            case R.id.rg_soft_decode:
                Log.e("xxx", "软解码");
                decode_type = false;
                break;
            case R.id.rb_render_texture:
                Log.e("xxx", "TextureView");
                render_type = Constants.RENDER_TEXTURE;
                break;
            case R.id.rb_render_surface:
                Log.e("xxx", "SurfaceView");
                render_type = Constants.RENDER_SURFACE;
                break;
            case R.id.rb_render_glsurface:
                Log.e("xxx", "GLSurfaceView");
                render_type = Constants.RENDER_GL_SURFACE;
                break;
            case R.id.rb_ijkPlayer:
                Log.e("xxx", "IjkPlayer");
                player_type = Constants.PLAYER_IJK;
                break;
            case R.id.rb_ObssPlayer:
                Log.e("xxx", "ObssPlayer");
                player_type = Constants.PLAYER_OBSS;
                break;
        }
    }
}

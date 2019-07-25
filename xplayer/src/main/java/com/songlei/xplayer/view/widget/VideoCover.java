package com.songlei.xplayer.view.widget;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.songlei.xplayer.R;


/**
 * 状态变化展示控件
 * Created by songlei on 2019/06/06.
 */
public class VideoCover extends RelativeLayout {
    private Context context;
    private View rootView;
    private RelativeLayout rl_no_wifi_tip;
    private TextView tv_keep_play;
    private CheckBox cb_no_more_tip;

    private LinearLayout ll_no_net;
    private TextView tv_refresh;
    private TextView tv_error_code;


    public VideoCover(Context context) {
        super(context);
    }

    public VideoCover(Context context, AttributeSet attrs) {
        super(context, attrs);
        this.context = context;
        initView();
    }

    private void initView(){
        rootView = LayoutInflater.from(context).inflate(R.layout.layout_video_cover, this);
        rl_no_wifi_tip = rootView.findViewById(R.id.rl_no_wifi_tip);
        tv_keep_play = rootView.findViewById(R.id.tv_keep_play);
        cb_no_more_tip = rootView.findViewById(R.id.cb_no_more_tip);

        ll_no_net = rootView.findViewById(R.id.ll_no_net);
        tv_refresh = rootView.findViewById(R.id.tv_refresh);
        tv_error_code = rootView.findViewById(R.id.tv_error_code);

        initListener();
    }

    private void initListener() {
        tv_keep_play.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                boolean isChecked = cb_no_more_tip.isChecked();
                onCoverListener.onNoWiFiKeepPlay(isChecked);
            }
        });
        tv_refresh.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                onCoverListener.onKeepPlay();
            }
        });
    }

    public void showNoWiFiTip(){
        setVisibility(VISIBLE);
        rl_no_wifi_tip.setVisibility(VISIBLE);
        ll_no_net.setVisibility(GONE);
    }

    public void showNoNet(int errorCode){
        setVisibility(VISIBLE);
        rl_no_wifi_tip.setVisibility(GONE);
        ll_no_net.setVisibility(VISIBLE);
        tv_error_code.setText("错误码：" + errorCode);
    }

    public void hide(){
        setVisibility(GONE);
        rl_no_wifi_tip.setVisibility(GONE);
        ll_no_net.setVisibility(GONE);
    }

    private OnCoverListener onCoverListener;

    public void setOnCoverListener(OnCoverListener onCoverListener){
        this.onCoverListener = onCoverListener;
    }

    public interface OnCoverListener{
        void onKeepPlay();

        void onNoWiFiKeepPlay(boolean isChecked);
    }

}

package com.songlei.xplayer.view.widget;

import android.app.Dialog;
import android.content.Context;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.TextView;

import androidx.annotation.NonNull;

import com.songlei.xplayer.R;
import com.songlei.xplayer.base.Option;
import com.songlei.xplayer.bean.VideoModeBean;
import com.songlei.xplayer.util.CommonUtil;

import java.util.Map;

/**
 * 切换分辨率对话框
 */
public class SwitchModeDialog extends Dialog {
    private RadioButton tvNormalClear;
    private RadioButton tvSuperClear;
    private RadioButton tvHighClear;
    private LinearLayout rlSwitchModeRoot;

    private View view;
    private Context mContext;
    private boolean isPortrait;

    private TextView tvMode;
    private OnSwitchModeListener onSwitchModeListener;
    private Map<Integer, VideoModeBean> videoModeMap;

    public SwitchModeDialog(@NonNull Context context, boolean isPortrait, TextView tvMode,
                            Map<Integer, VideoModeBean> videoModeMap, int defaultMode){
        this(context, isPortrait);
        this.tvMode = tvMode;
        this.videoModeMap = videoModeMap;
        if(videoModeMap.size() != 0){
            setModeVisiable();
            setMode(defaultMode);
        }
    }

    private SwitchModeDialog(@NonNull Context context, boolean isPortrait) {
        super(context, R.style.no_shadow_dialog);
        this.mContext = context;
        this.isPortrait = isPortrait;
        setContentView(initView());
        Window window = getWindow();
        window.setGravity(isPortrait ? Gravity.CENTER | Gravity.BOTTOM : Gravity.END);
        window.setWindowAnimations(isPortrait ? R.style.main_menu_animstyle : R.style.dialog_right_in);
        if (!isPortrait) {
            int flag = WindowManager.LayoutParams.FLAG_FULLSCREEN;
            window.setFlags(flag, flag);
            window.setType(WindowManager.LayoutParams.TYPE_APPLICATION_PANEL);
        }
        getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);  // 设置全屏显示，置于状态栏底下
        WindowManager.LayoutParams lp = window.getAttributes();
        lp.width = isPortrait ? ViewGroup.LayoutParams.MATCH_PARENT : CommonUtil.dip2px(context, 183);
        lp.height = isPortrait ? CommonUtil.dip2px(context,170) : CommonUtil.getScreenHeight(context);
        window.setAttributes(lp);
    }

    private View initView() {
        view = LayoutInflater.from(mContext).inflate(isPortrait ? R.layout.dialog_switch_mode : R.layout
                .dialog_switch_mode_land, null);
        tvNormalClear = view.findViewById(R.id.tv_normal_clear);
        tvSuperClear = view.findViewById(R.id.tv_super_clear);
        tvHighClear = view.findViewById(R.id.tv_high_clear);
        rlSwitchModeRoot = view.findViewById(R.id.rl_switch_mode_root);
        initListener();
        return view;
    }

    private void setMode(int mode){
        switch (mode){
            case Option.TYPE_MODE_NORMAL:
                tvNormalClear.setChecked(true);
                break;
            case Option.TYPE_MODE_SUPER_CLEAR:
                tvSuperClear.setChecked(true);
                break;
            case Option.TYPE_MODE_HIGH_CLEAR:
                tvHighClear.setChecked(true);
                break;
        }
    }

    private void setModeVisiable(){
        for (Map.Entry<Integer, VideoModeBean> entry : videoModeMap.entrySet()) {
            System.out.println("Key = " + entry.getKey() + ", Value = " + entry.getValue());
            switch (entry.getKey()) {
                case Option.TYPE_MODE_NORMAL:
                    tvNormalClear.setText(entry.getValue().show_name);
                    tvNormalClear.setVisibility(View.VISIBLE);
                    break;
                case Option.TYPE_MODE_SUPER_CLEAR:
                    tvSuperClear.setText(entry.getValue().show_name);
                    tvSuperClear.setVisibility(View.VISIBLE);
                    break;
                case Option.TYPE_MODE_HIGH_CLEAR:
                    tvHighClear.setText(entry.getValue().show_name);
                    tvHighClear.setVisibility(View.VISIBLE);
                    break;
            }
        }
    }

    private void initListener(){
        tvNormalClear.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean checked) {
                if(checked){
                    if(onSwitchModeListener != null){
                        onSwitchModeListener.onSwitchMode(Option.TYPE_MODE_NORMAL);
                    }
                    if(tvMode != null){
                        tvMode.setText(tvNormalClear.getText().toString().trim());
                    }
                }
                dismiss();
            }
        });
        tvSuperClear.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean checked) {
                if(checked){
                    if(onSwitchModeListener != null){
                        onSwitchModeListener.onSwitchMode(Option.TYPE_MODE_SUPER_CLEAR);
                    }
                    if(tvMode != null){
                        tvMode.setText(tvSuperClear.getText().toString().trim());
                    }
                }
                dismiss();
            }
        });
        tvHighClear.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean checked) {
                if(checked){
                    if(onSwitchModeListener != null){
                        onSwitchModeListener.onSwitchMode(Option.TYPE_MODE_HIGH_CLEAR);
                    }
                    if(tvMode != null){
                        tvMode.setText(tvHighClear.getText().toString().trim());
                    }
                }
                dismiss();
            }
        });
//        tvStandardClear.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
//            @Override
//            public void onCheckedChanged(CompoundButton compoundButton, boolean checked) {
//                if(checked){
////                    if(onSwitchModeListener != null){
////                        onSwitchModeListener.onSwitchMode(WatchConstants.SWITCH_MODE_STANDARD_CLEAR);
////                    }
//                    if(tvMode != null){
//                        tvMode.setText("标清");
//                    }
//                }
//                dismiss();
//            }
//        });
//        rlSwitchModeRoot.setOnClickListener(this);
    }

//    @Override
//    public void onClick(View v) {
//        switch (v.getId()) {
//            case R.id.rl_switch_mode_root:
//                dismiss();
//                break;
//        }
//    }

    public interface OnSwitchModeListener{
        void onSwitchMode(int mode);
    }

    public void setOnSwitchModeListener(OnSwitchModeListener onSwitchModeListener){
        this.onSwitchModeListener = onSwitchModeListener;
    }
}

package com.songlei.ppplayerdemo.adapter;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;

import com.songlei.ppplayerdemo.R;
import com.songlei.ppplayerdemo.bean.VideoModel;
import com.songlei.ppplayerdemo.util.SwitchUtil;
import com.songlei.ppplayerdemo.view.SwitchVideo;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by songlei on 2019/08/02.
 */
public class SwitchListVideoAdapter extends BaseAdapter {
    private List<VideoModel> list = new ArrayList<>();
    private LayoutInflater inflater;
    private Context context;

    public SwitchListVideoAdapter(Context context){
        this.context = context;
        inflater = LayoutInflater.from(context);
        for (int i = 0; i < 40; i++) {
            list.add(new VideoModel());
        }
    }

    @Override
    public int getCount() {
        return list.size();
    }

    @Override
    public Object getItem(int position) {
        return null;
    }

    @Override
    public long getItemId(int position) {
        return 0;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder holder;
        if (convertView == null) {
            holder = new ViewHolder();
            convertView = inflater.inflate(R.layout.switch_list_video_item, null);
            holder.switchVideo = convertView.findViewById(R.id.video_item_player);
            holder.imageView = new ImageView(context);
            convertView.setTag(holder);
        } else {
            holder = (ViewHolder) convertView.getTag();
        }
        int coverId = (position % 2 == 0) ? R.mipmap.xxx1 : R.mipmap.xxx2;

        holder.imageView.setScaleType(ImageView.ScaleType.CENTER_CROP);
        holder.imageView.setImageResource(coverId);
        if (holder.imageView.getParent() != null) {
            ViewGroup viewGroup = (ViewGroup) holder.imageView.getParent();
            viewGroup.removeView(holder.imageView);
        }
        holder.switchVideo.setThumbImageView(holder.imageView);

        String urlH = "http://9890.vod.myqcloud.com/9890_4e292f9a3dd011e6b4078980237cc3d3.f20.mp4";
        String urlV = "http://wdquan-space.b0.upaiyun.com/VIDEO/2018/11/22/ae0645396048_hls_time10.m3u8";
        String url = (position % 2 == 0) ? urlH : urlV;
        SwitchUtil.optionPlayer(holder.switchVideo, url, "你大爷");
        holder.switchVideo.setPlayPosition(position);

        holder.switchVideo.getThumbImageViewLayout().setVisibility(View.VISIBLE);
        return convertView;
    }

    class ViewHolder {
        SwitchVideo switchVideo;
        ImageView imageView;
    }
}

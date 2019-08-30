package com.songlei.slplayer.util;

import android.media.MediaCodecList;
import android.util.Log;

import java.util.HashMap;
import java.util.Map;

/**
 * 硬解码工具类
 * Created by songlei on 2019/03/19.
 */
public class MediaDecodeUtil {

    private static Map<String, String> codecMap = new HashMap<>();
    static {
        codecMap.put("h264", "video/avc");
    }

    public static String getMediaDecodeName(String codecName){
        if (codecMap.containsKey(codecName)) {
            return codecMap.get(codecName);
        }
        return "";
    }

    public static boolean isSupportDecoder(String codecName){
        int count = MediaCodecList.getCodecCount();
        for (int i = 0; i < count; i++) {
            String[] supportedTypes = MediaCodecList.getCodecInfoAt(i).getSupportedTypes();
            for (int j = 0; j < supportedTypes.length; j++) {
                Log.e("jj", "support type = " + supportedTypes[j]);
                if (supportedTypes[j].equals(getMediaDecodeName(codecName))){
                    return true;
                }
            }
        }
        return false;
    }

}

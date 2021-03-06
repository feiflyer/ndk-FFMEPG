package com.fast.flyer.ffmpeg.utils;

public final class VideoUtils {

    public static final String VIDEO_IN_PATH = "/mnt/sdcard/Download/1080.mp4";
    public static final String VIDEO_OUT_H264_PATH = "/mnt/sdcard/Download/1080.h264";
    public static final String VIDEO_OUT_YUV_PATH = "/mnt/sdcard/Download/1080.yuv";

    static {
        System.loadLibrary("native-lib");
    }

    public static boolean decodeVideoforH264(){
        return decodeVieoForH264(VIDEO_IN_PATH,VIDEO_OUT_H264_PATH);
    }

    public static boolean decodeVideoforYUV(){
        return decodeVieoForYUV(VIDEO_IN_PATH,VIDEO_OUT_YUV_PATH);
    }

    public static native boolean decodeVieoForH264(String inPath,String outPath);

    public static native boolean decodeVieoForYUV(String inPath,String outPath);

    public static native boolean changeVolume(String inPath,String outPath, float volume);


    public static native boolean mixAudio(String inPath,String inPath2, String outPath);

    public static native boolean mixAudioWithVolume(String bgmPath,String recordPath, String outPath,String bgmVolume,String recordVolume);

    public static native byte[] mixAudio(byte[] bgmBytes,float bgmVolume, byte[] recordBytes, float recordVolume);
}

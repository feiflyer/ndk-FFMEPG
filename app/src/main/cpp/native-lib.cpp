#include <jni.h>
#include <string>
#include "LOGUtils.h"

extern "C"{

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"

}
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_fast_flyer_ffmpeg_utils_VideoUtils_decodeVieoForH264(JNIEnv *env, jclass type,
                                                              jstring inPath_, jstring outPath_) {
    const char *inPath = env->GetStringUTFChars(inPath_, 0);
    const char *outPath = env->GetStringUTFChars(outPath_, 0);

    AVFormatContext *pFormatCtx;

    int videoindex = -1;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;

    AVPacket *packet;

    FILE *fp_h264=fopen(outPath,"wb+");

    av_register_all();//注册所有组件
    avformat_network_init();//初始化网络
    pFormatCtx = avformat_alloc_context();//初始化一个AVFormatContext

    if(avformat_open_input(&pFormatCtx,inPath,NULL,NULL)!=0){//打开输入的视频文件

        LOGW("Couldn't open input stream");

        return false;
    }

    if(avformat_find_stream_info(pFormatCtx,NULL)<0){//获取视频文件信息
        LOGW("Couldn't find stream information");
        return false;
    }

    videoindex = av_find_best_stream(pFormatCtx,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);


    if(videoindex == -1){
        LOGW("Didn't find a video stream");
        return false;
    }



    pCodec = avcodec_find_decoder(pFormatCtx->streams[videoindex]->codecpar->codec_id);//查找解码器

    if(pCodec==NULL){
        LOGW("Codec not found");
        return false;
    }

    //解码器初始化
    pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecCtx,pFormatCtx->streams[videoindex]->codecpar);

    pCodecCtx->thread_count = 8;


    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){//打开解码器
        LOGW("Could not open codec.\n");
        return false;
    }

    packet=(AVPacket *)av_malloc(sizeof(AVPacket));

    while(av_read_frame(pFormatCtx, packet) >= 0){//读取一帧压缩数据
        if(packet->stream_index == videoindex){
            fwrite(packet->data,1,packet->size,fp_h264); //把H264数据写入fp_h264文件
        }
    }
    av_packet_unref(packet);


    fclose(fp_h264);


    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    env->ReleaseStringUTFChars(inPath_, inPath);
    env->ReleaseStringUTFChars(outPath_, outPath);

    LOGW("解码h264成功");
    return true;
}



extern "C"
JNIEXPORT jboolean JNICALL
Java_com_fast_flyer_ffmpeg_utils_VideoUtils_decodeVieoForYUV(JNIEnv *env, jclass type,
                                                             jstring inPath_, jstring outPath_) {
    const char *inPath = env->GetStringUTFChars(inPath_, 0);
    const char *outPath = env->GetStringUTFChars(outPath_, 0);


    env->ReleaseStringUTFChars(inPath_, inPath);
    env->ReleaseStringUTFChars(outPath_, outPath);

    return false;
}
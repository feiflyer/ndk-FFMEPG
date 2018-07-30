#include <jni.h>
#include <string>
#include "LOGUtils.h"

extern "C" {

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_fast_flyer_ffmpeg_utils_VideoUtils_decodeVieoForH264(JNIEnv *env, jclass type,
                                                              jstring inPath_, jstring outPath_) {
    const char *inPath = env->GetStringUTFChars(inPath_, 0);
    const char *outPath = env->GetStringUTFChars(outPath_, 0);

    AVFormatContext *pFormatCtx;

    int videoindex = -1;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;

    AVPacket *packet;

    FILE *fp_h264 = fopen(outPath, "wb+");

    av_register_all();//注册所有组件
    avformat_network_init();//初始化网络
    pFormatCtx = avformat_alloc_context();//初始化一个AVFormatContext

    if (avformat_open_input(&pFormatCtx, inPath, NULL, NULL) != 0) {//打开输入的视频文件

        LOGW("Couldn't open input stream");

        return false;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {//获取视频文件信息
        LOGW("Couldn't find stream information");
        return false;
    }

    videoindex = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);


    if (videoindex == -1) {
        LOGW("Didn't find a video stream");
        return false;
    }


    pCodec = avcodec_find_decoder(pFormatCtx->streams[videoindex]->codecpar->codec_id);//查找解码器

    if (pCodec == NULL) {
        LOGW("Codec not found");
        return false;
    }

    //解码器初始化
    pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoindex]->codecpar);

    pCodecCtx->thread_count = 8;


    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {//打开解码器
        LOGW("Could not open codec.\n");
        return false;
    }

    packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    while (av_read_frame(pFormatCtx, packet) >= 0) {//读取一帧压缩数据
        if (packet->stream_index == videoindex) {
            fwrite(packet->data, 1, packet->size, fp_h264); //把H264数据写入fp_h264文件
        }
        av_packet_unref(packet);
    }


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

    AVFormatContext *pFormatCtx;

    int videoindex = -1;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;

    AVPacket *packet;

    FILE *fp_yuv = fopen(outPath, "wb+");

    av_register_all();//注册所有组件
    avformat_network_init();//初始化网络
    pFormatCtx = avformat_alloc_context();//初始化一个AVFormatContext

    if (avformat_open_input(&pFormatCtx, inPath, NULL, NULL) != 0) {//打开输入的视频文件

        LOGW("Couldn't open input stream");

        return false;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {//获取视频文件信息
        LOGW("Couldn't find stream information");
        return false;
    }

    videoindex = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);


    if (videoindex == -1) {
        LOGW("Didn't find a video stream");
        return false;
    }


    pCodec = avcodec_find_decoder(pFormatCtx->streams[videoindex]->codecpar->codec_id);//查找解码器

    if (pCodec == NULL) {
        LOGW("Codec not found");
        return false;
    }

    //解码器初始化
    pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoindex]->codecpar);

    pCodecCtx->thread_count = 8;


    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {//打开解码器
        LOGW("Could not open codec.\n");
        return false;
    }

    packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    //初始化像素格式转换的上下文
    SwsContext *vctx = NULL;

    AVFrame *frame = av_frame_alloc();

    int re = -1;

    while ((re = av_read_frame(pFormatCtx, packet)) >= 0) {//读取一帧压缩数据

        if (packet->stream_index == videoindex) {

            //发送到线程去解码
            re = avcodec_send_packet(pCodecCtx, packet);

            if (re < 0) {
                LOGW("avcodec_send_packet fail");
            }

            re = avcodec_receive_frame(pCodecCtx, frame);

            while (re >= 0) {

                if (re == AVERROR(EAGAIN) || re == AVERROR_EOF){
                    break;
                }

                vctx = sws_getCachedContext(vctx,
                                            frame->width,
                                            frame->height,
                                            (AVPixelFormat) frame->format,
                                            frame->width,
                                            frame->height,
                                            AV_PIX_FMT_YUV420P,
                                            SWS_FAST_BILINEAR,
                                            0, 0, 0);

                if (!vctx) {
                    LOGW("sws_getCachedContext fail");
                    return false;
                }

                int size = frame->width * frame->width;

                fwrite(frame->data[0], 1, size, fp_yuv);    //Y
                fwrite(frame->data[1], 1, size / 4, fp_yuv);  //U
                fwrite(frame->data[2], 1, size / 4, fp_yuv);  //V

                re = avcodec_receive_frame(pCodecCtx, frame);
            }
        }
        av_packet_unref(packet);
    }
    av_packet_free(&packet);

    fclose(fp_yuv);
    av_frame_free(&frame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    env->ReleaseStringUTFChars(inPath_, inPath);
    env->ReleaseStringUTFChars(outPath_, outPath);

    LOGW("解码YUV成功");

    return true;
}
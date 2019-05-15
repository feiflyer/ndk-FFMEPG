#include <jni.h>
#include <string>
#include "LOGUtils.h"

#include "ffmpeg_filter.h"
#include "hash_compare.h"
#include "amix_filter.h"
#include "volume_filter.h"


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


extern "C"
JNIEXPORT jboolean JNICALL
Java_com_fast_flyer_ffmpeg_utils_VideoUtils_changeVolume(JNIEnv *env, jclass type, jstring inPath_,
                                                         jstring outPath_, jfloat volume) {

    const char *inPath = env->GetStringUTFChars(inPath_, 0);
    const char *outPath = env->GetStringUTFChars(outPath_, 0);

    int ret;
    Frame *in_frame, *out_frame;
    size_t max_len = 1024 * 2 * 2; //nb_sample * s16所占位数 * 声道数
    Filter *filter;
    int len = 0;

    FILE *out_file;
    int8_t *buf = static_cast<int8_t *>(malloc(max_len));





    out_file = fopen(outPath, "wb+");

    filter = find_filter_by_name("volume");


    ret = filter->set_option_value("volume", "2");

    ret = filter->set_option_value("format", "s16");

    ret = filter->set_option_value("channel", "2");

    ret = filter->set_option_value("rate", "44100");


    ret = filter->init();

    in_frame = malloc_frame(AUDIO);

    FILE* inputFile = fopen(inPath,"r");


    while ((len = fread(buf, sizeof(int8_t),max_len,inputFile)) > 0)
    {

        frame_set_data(in_frame, buf, static_cast<const size_t>(len));
        filter->add_frame(in_frame);
        out_frame = filter->get_frame();
        if (out_frame)
        {
            fwrite(out_frame->data[0], 1, static_cast<size_t>(out_frame->linesize[0]), out_file);
            free_frame(&out_frame);
        }
    }

    fclose(inputFile);

    free(buf);
    free_frame(&in_frame);
    fclose(out_file);
    free_filter(&filter);
    env->ReleaseStringUTFChars(inPath_, inPath);
    env->ReleaseStringUTFChars(outPath_, outPath);

    return true;
}

unsigned char *mixAudio(unsigned char *bgmchar, float bgmVolume, unsigned char *bgmChar, float recordVolume)
{

//    unsigned char *bgmChar = static_cast<unsigned char *>(malloc(sizeof(unsigned char) * bgmSize));

    return 0;

}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_fast_flyer_ffmpeg_utils_VideoUtils_mixAudio(JNIEnv *env, jclass type, jstring inPath_,
                                                     jstring inPath2_, jstring outPath_) {
    const char *inPath = env->GetStringUTFChars(inPath_, 0);
    const char *inPath2 = env->GetStringUTFChars(inPath2_, 0);
    const char *outPath = env->GetStringUTFChars(outPath_, 0);

    int ret;

    Frame *in_frame0, *in_frame1, *out_frame;
    size_t max_len = 1024 * 2 * 2;
    Filter *filter;
    int len0 = 0, len1 = 0;
    FILE *out_file;

    int8_t *buf0 = static_cast<int8_t *>(malloc(max_len));
    int8_t *buf1 = static_cast<int8_t *>(malloc(max_len));


    out_file = fopen(outPath, "wb+");


    filter = find_filter_by_name("amix");


    ret = filter->set_option_value("format_in0", "s16");

    ret = filter->set_option_value("channel_in0", "2");

    ret = filter->set_option_value("rate_in0", "44100");

    ret = filter->set_option_value("format_in1", "s16");

    ret = filter->set_option_value("channel_in1", "2");

    ret = filter->set_option_value("rate_in1", "44100");

    ret = filter->set_option_value("format_out", "s16");

    ret = filter->set_option_value("channel_out", "2");

    ret = filter->set_option_value("rate_out", "44100");


    ret = filter->init();

    in_frame0 = malloc_frame(AUDIO);
    in_frame1 = malloc_frame(AUDIO);


    FILE* inputFile0 = fopen(inPath,"r");

    FILE* inputFile1 = fopen(inPath2,"r");


    while ((len0 = fread(buf0, sizeof(int8_t),max_len,inputFile0)) > 0 && (len1 = fread(buf1, sizeof(int8_t),max_len,inputFile1)) > 0)
    {
        frame_set_data(in_frame0, buf0, static_cast<const size_t>(len0));

        filter->add_frame(in_frame0, 0);

        frame_set_data(in_frame1, buf1, static_cast<const size_t>(len1));
        filter->add_frame(in_frame1, 1);
        out_frame = filter->get_frame();
        if (out_frame)
        {
            fwrite(out_frame->data[0], 1, static_cast<size_t>(out_frame->linesize[0]), out_file);
            free_frame(&out_frame);
        }
    }

    free(buf0);
    free(buf1);
    free_frame(&in_frame0);
    free_frame(&in_frame1);
    fclose(inputFile0);
    fclose(inputFile1);
    fclose(out_file);
    free_filter(&filter);

    env->ReleaseStringUTFChars(inPath_, inPath);
    env->ReleaseStringUTFChars(inPath2_, inPath2);
    env->ReleaseStringUTFChars(outPath_, outPath);

    return true;
}

extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_fast_flyer_ffmpeg_utils_VideoUtils_mixAudio___3BF_3BF(JNIEnv *env, jclass type,
                                                               jbyteArray bgmBytes_,
                                                               jfloat bgmVolume,
                                                               jbyteArray recordBytes_,
                                                               jfloat recordVolume) {

    jbyte *bgmBytes = env->GetByteArrayElements(bgmBytes_, NULL);
    jbyte *recordBytes = env->GetByteArrayElements(recordBytes_, NULL);


    mixAudio(reinterpret_cast<unsigned char *>(bgmBytes), bgmVolume, reinterpret_cast<unsigned char *>(recordBytes), recordVolume);


    env->ReleaseByteArrayElements(bgmBytes_, bgmBytes, 0);
    env->ReleaseByteArrayElements(recordBytes_, recordBytes, 0);
}



extern "C"
JNIEXPORT jboolean JNICALL
Java_com_fast_flyer_ffmpeg_utils_VideoUtils_mixAudioWithVolume(JNIEnv *env, jclass type,
                                                               jstring bgmPath_,
                                                               jstring recordPath_,
                                                               jstring outPath_, jstring bgmVolume_,
                                                               jstring recordVolume_) {

    const char *bgmPath = env->GetStringUTFChars(bgmPath_, 0);
    const char *recordPath = env->GetStringUTFChars(recordPath_, 0);
    const char *outPath = env->GetStringUTFChars(outPath_, 0);
    const char *bgmVolume = env->GetStringUTFChars(bgmVolume_, 0);
    const char *recordVolume = env->GetStringUTFChars(recordVolume_, 0);


    int ret;

    //初始化音量过滤器
    Filter *bgm_volume_filter;

    bgm_volume_filter = find_filter_by_name("volume");


    ret = bgm_volume_filter->set_option_value("volume", bgmVolume);

    ret = bgm_volume_filter->set_option_value("format", "s16");

    ret = bgm_volume_filter->set_option_value("channel", "2");

    ret = bgm_volume_filter->set_option_value("rate", "44100");


    ret = bgm_volume_filter->init();


    Filter *record_volume_filter;

    record_volume_filter = find_filter_by_name("volume");


    ret = record_volume_filter->set_option_value("volume", recordVolume);

    ret = record_volume_filter->set_option_value("format", "s16");

    ret = record_volume_filter->set_option_value("channel", "2");

    ret = record_volume_filter->set_option_value("rate", "44100");

    ret = record_volume_filter->init();

//    in_frame = malloc_frame(AUDIO);
//
//    FILE* inputFile = fopen(inPath,"r");


//    while ((len = fread(buf, sizeof(int8_t),max_len,inputFile)) > 0)
//    {
//
//        frame_set_data(in_frame, buf, static_cast<const size_t>(len));
//        filter->add_frame(in_frame);
//        out_frame = filter->get_frame();
//        if (out_frame)
//        {
//            fwrite(out_frame->data[0], 1, static_cast<size_t>(out_frame->linesize[0]), out_file);
//            free_frame(&out_frame);
//        }
//    }
//
//    fclose(inputFile);
//
//    free(buf);
//    free_frame(&in_frame);
//    fclose(out_file);
//    free_filter(&filter);
//    env->ReleaseStringUTFChars(inPath_, inPath);
//    env->ReleaseStringUTFChars(outPath_, outPath);


    // TODO

    //初始化混音过滤器
    Filter *amixFilter;
    amixFilter = find_filter_by_name("amix");

    ret = amixFilter->set_option_value("format_in0", "s16");

    ret = amixFilter->set_option_value("channel_in0", "2");

    ret = amixFilter->set_option_value("rate_in0", "44100");

    ret = amixFilter->set_option_value("format_in1", "s16");

    ret = amixFilter->set_option_value("channel_in1", "2");

    ret = amixFilter->set_option_value("rate_in1", "44100");

    ret = amixFilter->set_option_value("format_out", "s16");

    ret = amixFilter->set_option_value("channel_out", "2");

    ret = amixFilter->set_option_value("rate_out", "44100");

    ret = amixFilter->init();


    Frame *in_bgm_frame, *in_record_frame, *out_bgm_frame,*out_record_frame,*out_mix_frame;

    size_t max_len = 1024 * 2 * 2;

    int bgmLen = 0, recordLen = 0;

    FILE *out_file;

    int8_t *bgmBuf = static_cast<int8_t *>(malloc(max_len));

    int8_t *recordBuf = static_cast<int8_t *>(malloc(max_len));

    out_file = fopen(outPath, "wb+");

    in_bgm_frame = malloc_frame(AUDIO);

    in_record_frame = malloc_frame(AUDIO);

    FILE* bgmFILE = fopen(bgmPath,"r");

    FILE* recordFILE = fopen(recordPath,"r");


    LOGW("aaaaa--while");

    while ((bgmLen = fread(bgmBuf, sizeof(int8_t),max_len,bgmFILE)) > 0 && (recordLen = fread(recordBuf, sizeof(int8_t),max_len,recordFILE)) > 0 && bgmLen == recordLen)
    {

       LOGW("aaaaa--while");
        //改变音量

        frame_set_data(in_bgm_frame, bgmBuf, static_cast<const size_t>(bgmLen));
        bgm_volume_filter->add_frame(in_bgm_frame);
        out_bgm_frame = bgm_volume_filter->get_frame();


        frame_set_data(in_record_frame, recordBuf, static_cast<const size_t>(recordLen));
        record_volume_filter->add_frame(in_record_frame);
        out_record_frame = record_volume_filter->get_frame();


        if(out_bgm_frame && out_record_frame){


            //混音

            frame_set_data(in_bgm_frame, out_bgm_frame->data[0], static_cast<size_t>(out_bgm_frame->linesize[0]));

            amixFilter->add_frame(in_bgm_frame, 0);

            frame_set_data(in_record_frame, out_record_frame->data[0], static_cast<size_t>(out_record_frame->linesize[0]));

            amixFilter->add_frame(in_record_frame, 1);

            out_mix_frame = amixFilter->get_frame();


            free_frame(&out_bgm_frame);
            free_frame(&out_record_frame);

            if (out_mix_frame)
            {
                fwrite(out_mix_frame->data[0], 1, static_cast<size_t>(out_mix_frame->linesize[0]), out_file);
                free_frame(&out_mix_frame);
            }

        }


    }


    LOGW("aaaaa--free");

    free(bgmBuf);
    free(recordBuf);

    free_frame(&in_bgm_frame);
    free_frame(&in_record_frame);

    fclose(bgmFILE);
    fclose(recordFILE);
    fclose(out_file);

    free_filter(&bgm_volume_filter);
    free_filter(&record_volume_filter);
    free_filter(&amixFilter);


    env->ReleaseStringUTFChars(bgmPath_, bgmPath);
    env->ReleaseStringUTFChars(recordPath_, recordPath);
    env->ReleaseStringUTFChars(outPath_, outPath);
    env->ReleaseStringUTFChars(bgmVolume_, bgmVolume);
    env->ReleaseStringUTFChars(recordVolume_, recordVolume);

    return true;
}
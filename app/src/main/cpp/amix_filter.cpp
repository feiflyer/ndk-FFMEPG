

#include "hash_compare.h"
#include "utils.h"

#include "amix_filter.h"

AmixFilter::AmixFilter()
: sample_format_in0(AV_SAMPLE_FMT_S16)
, sample_format_in1(AV_SAMPLE_FMT_S16)
, sample_format_out(AV_SAMPLE_FMT_S16)
, sample_channel_in0(AV_CH_LAYOUT_MONO)
, sample_channel_in1(AV_CH_LAYOUT_MONO)
, sample_channel_out(AV_CH_LAYOUT_MONO)
, sample_rate_in0(44100)
, sample_rate_in1(44100)
, sample_rate_out(44100)
, graph(nullptr)
, abuffersrc0_ctx(nullptr)
, abuffersrc1_ctx(nullptr)
, abuffersink_ctx(nullptr)
, aformat_ctx(nullptr)
, amix_ctx(nullptr)
, init_success(false)
{

}

extern "C"
AmixFilter::~AmixFilter()
{
    avfilter_free(abuffersrc0_ctx);
    avfilter_free(abuffersrc1_ctx);
    avfilter_free(abuffersink_ctx);
    avfilter_free(aformat_ctx);
    avfilter_free(amix_ctx);
    avfilter_graph_free(&graph);
}

int AmixFilter::set_option_value(std::string key, std::string vaule)
{
    int result = 0;
    switch (hash(key.c_str())) {
        case "format_in0"_hash:
            sample_format_in0 = strcmp(vaule.c_str(), "u8") == 0 ?  AV_SAMPLE_FMT_U8 : strcmp(vaule.c_str(), "s32") == 0 ? AV_SAMPLE_FMT_S32 : AV_SAMPLE_FMT_S16;
            break;
        case "format_in1"_hash:
            sample_format_in1 = strcmp(vaule.c_str(), "u8") == 0 ?  AV_SAMPLE_FMT_U8 : strcmp(vaule.c_str(), "s32") == 0 ? AV_SAMPLE_FMT_S32 : AV_SAMPLE_FMT_S16;
            break;
        case "format_out"_hash:
            sample_format_out = strcmp(vaule.c_str(), "u8") == 0 ?  AV_SAMPLE_FMT_U8 : strcmp(vaule.c_str(), "s32") == 0 ? AV_SAMPLE_FMT_S32 : AV_SAMPLE_FMT_S16;
            break;
        case "channel_in0"_hash:
            sample_channel_in0 = strcmp(vaule.c_str(), "2") == 0 ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;
            break;
        case "channel_in1"_hash:
            sample_channel_in1 = strcmp(vaule.c_str(), "2") == 0 ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;
            break;
        case "channel_out"_hash:
            sample_channel_out = strcmp(vaule.c_str(), "2") == 0 ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;
            break;
        case "rate_in0"_hash:
            sample_rate_in0 = atoi(vaule.c_str());
            break;
        case "rate_in1"_hash:
            sample_rate_in1 = atoi(vaule.c_str());
            break;
        case "rate_out"_hash:
            sample_rate_out = atoi(vaule.c_str());
            break;
        default:
            result = -1;
    }
    return result;
}

int AmixFilter::init()
{
    int ret;
    AVFilter *abuffersrc0, *abuffersrc1, *abuffersink, *amix, *aformat;
    char options_str[1024];
    avfilter_register_all();

    graph = avfilter_graph_alloc();
    CHECK_NULL(graph);

    //根据 abuffer 获取第一个音频输入端filter
    abuffersrc0 = avfilter_get_by_name("abuffer");
    CHECK_NULL(abuffersrc0);
    //获取该filter的上下文环境
    abuffersrc0_ctx = avfilter_graph_alloc_filter(graph, abuffersrc0, "src0");
    CHECK_NULL(abuffersrc0_ctx);
    //构造参数配置（输入音频格式、输入采样率、输入声道配置）
    snprintf(options_str, sizeof(options_str),
             "sample_fmt=%s:sample_rate=%d:channel_layout=0x%" PRIx64 ,
             av_get_sample_fmt_name(sample_format_in0),
             sample_rate_in0,
             sample_channel_in0);
    //传入参数开始初始化
    ret = avfilter_init_str(abuffersrc0_ctx, options_str);
    CHECK_RESULT(ret);

    //根据 abuffer 获取第二个音频输入端filter
    abuffersrc1 = avfilter_get_by_name("abuffer");
    CHECK_NULL(abuffersrc1);
    //获取该filter的上下文环境
    abuffersrc1_ctx = avfilter_graph_alloc_filter(graph, abuffersrc1, "src1");
    CHECK_NULL(abuffersrc1_ctx);
    //构造参数配置（输入音频格式、输入采样率、输入声道配置）
    snprintf(options_str, sizeof(options_str),
             "sample_fmt=%s:sample_rate=%d:channel_layout=0x%" PRIx64 ,
             av_get_sample_fmt_name(sample_format_in1),
             sample_rate_in1,
             sample_channel_in1);
    //传入参数开始初始化
    ret = avfilter_init_str(abuffersrc1_ctx, options_str);
    CHECK_RESULT(ret);

    //根据 amix 获取混音处理的filter
    amix = avfilter_get_by_name("amix");
    CHECK_NULL(amix);
    //获取该filter的上下文环境
    amix_ctx = avfilter_graph_alloc_filter(graph, amix, "amix");
    CHECK_NULL(amix_ctx);
    //传入参数开始初始化，这个不需要指定参数
    ret = avfilter_init_str(amix_ctx, NULL);
    CHECK_RESULT(ret);

    //根据 aformat 获取一个音频格式转换的filter
    aformat = avfilter_get_by_name("aformat");
    CHECK_NULL(aformat);
    //获取该filter的上下文环境
    aformat_ctx = avfilter_graph_alloc_filter(graph, aformat, "aformat");
    CHECK_NULL(aformat_ctx);
    //构造参数配置（输出音频格式、输出采样率、输出声道配置）
    snprintf(options_str, sizeof(options_str),
             "sample_fmts=%s:sample_rates=%d:channel_layouts=0x%" PRIx64 ,
             av_get_sample_fmt_name(sample_format_out),
             sample_rate_out,
             sample_channel_out);
    //传入参数开始初始化
    ret = avfilter_init_str(aformat_ctx, options_str);
    CHECK_RESULT(ret);

    //根据 abuffersink 获取一个音频数据输出的filter
    abuffersink = avfilter_get_by_name("abuffersink");
    CHECK_NULL(abuffersink);
    //获取该filter的上下文环境
    abuffersink_ctx = avfilter_graph_alloc_filter(graph, abuffersink, "sink");
    CHECK_NULL(abuffersink_ctx);
    //传入参数开始初始化，这个不需要指定参数
    ret = avfilter_init_str(abuffersink_ctx, NULL);
    CHECK_RESULT(ret);

    //开始链接filter,这里业务逻辑的具体顺序是
    // abuffer0 -> (aformat0)
    //                        -> amix -> aformat -> abuffersink
    // abuffer1 -> (aformat0)
    //(aformat)由于filter会默认在 abuffer -> amix 会根据输入情况插入一个格式转换filter导致输出格式变化
    //需要我们需要在最后添加一个aformat将格式转换为我们期望的输出格式
    ret = avfilter_link(abuffersrc0_ctx, 0, amix_ctx, 0);
    CHECK_RESULT(ret);
    ret = avfilter_link(abuffersrc1_ctx, 0, amix_ctx, 1);
    CHECK_RESULT(ret);
    ret = avfilter_link(amix_ctx, 0, aformat_ctx, 0);
    CHECK_RESULT(ret);
    ret = avfilter_link(aformat_ctx, 0, abuffersink_ctx, 0);
    CHECK_RESULT(ret);

    //初始化整个filter链，初始化完毕
    ret = avfilter_graph_config(graph, NULL);
    CHECK_RESULT(ret);

    init_success = true;

    return 0;
}

int AmixFilter::add_frame(Frame *frame, int index)
{
    AVFrame *avframe;
    int ret;
    if (!init_success)
        return -1;
    int sample_rate = index == 0 ? sample_rate_in0 : sample_rate_in1;
    uint64_t sample_channel = index == 0 ? sample_channel_in0 : sample_channel_in1;
    AVSampleFormat sample_format = index == 0 ? sample_format_in0 : sample_format_in1;

    //采样数计算 = 数据总长度 / (对应采样率所占位数 * 声道数)  PS:这里仅处理多声道在一个平面的情况
    int nb_sample = frame->linesize[0] / av_get_bytes_per_sample(sample_format) / (sample_channel == AV_CH_LAYOUT_MONO ? 1 : 2);

    //获取一个AVFrame实例
    avframe = av_frame_alloc();
    CHECK_NULL_GOTO(avframe, err);

    //配置AVFrame的音频格式信息
    avframe->sample_rate = sample_rate;
    avframe->format = sample_format;
    avframe->channel_layout = sample_channel;
    avframe->nb_samples = nb_sample;

    //申请AVFrame的音频数据内存空间
    ret = av_frame_get_buffer(avframe, 1);
    CHECK_RESULT_GOTO(ret, err);

    //将外部输入数据复制到AVFrame
    memcpy(avframe->extended_data[0], frame->data[0], static_cast<size_t>(frame->linesize[0]));

    //将AVFrame传输到音频接收filter的上下文环境中
    ret = av_buffersrc_add_frame(index == 0 ? abuffersrc0_ctx : abuffersrc1_ctx, avframe);
    CHECK_RESULT_GOTO(ret, err);

    //我们不需要这个AVFrame了，需要释放
    av_frame_free(&avframe);
    return 0;

err:
    av_frame_free(&avframe);
    return -1;
}

Frame *AmixFilter::get_frame(int index)
{
    AVFrame *avframe;
    Frame *frame;
    int ret, size;
    int read_size = 0;
    if (!init_success)
        return nullptr;

    //获取一个AVFrame实例
    //我们并不需要为此设置该AVFrame的音频格式参数，也不需要申请AVFrame的音频数据内存空间
    avframe = av_frame_alloc();
    CHECK_NULL_GOTO(avframe, err);

    //利用AVFrame从abuffersink的上下文环境中获取处理后的音频数据包
    ret = av_buffersink_get_frame(abuffersink_ctx, avframe);
    CHECK_RESULT_GOTO(ret, err);

    frame = malloc_frame(AUDIO);
    //av_samples_get_buffer_size 可以算出音频数据所需要的内存长度
    //并不能直接用 avframe->linesize[0],因为这个存在对齐的情况导致数据偏大
    size = av_samples_get_buffer_size(NULL, (sample_channel_out == AV_CH_LAYOUT_MONO ? 1 : 2), avframe->nb_samples, sample_format_out, 1);
    CHECK_RESULT_GOTO(read_size, err);
    frame_set_data(frame, avframe->extended_data[0], size);

    //我们不需要这个AVFrame了，需要释放
    av_frame_free(&avframe);
    return frame;

    err:
    av_frame_free(&avframe);
    return nullptr;
}
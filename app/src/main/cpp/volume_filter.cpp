

#include "volume_filter.h"
#include "utils.h"
#include "hash_compare.h"




VolumeFilter::VolumeFilter()
: graph(nullptr)
, abuffersrc_ctx(nullptr)
, volume_ctx(nullptr)
, aformat_ctx(nullptr)
, abuffersink_ctx(nullptr)
, sample_rate(44100)
, sample_channel(AV_CH_LAYOUT_MONO)
, sample_format(AV_SAMPLE_FMT_S16)
, volme_value("1.0")
, init_success(false)
{

}

extern "C"
int VolumeFilter::init()
{
    int ret;
    AVFilter *abuffersrc, *abuffersink, *volume, *aformat;
    AVDictionary *options_dict = NULL;
    char ch_layout[64];
    char options_str[1024];

    //注册全部ffmpeg filter
    avfilter_register_all();

    //实例化一个 filter graph，这个是调用 filter必须获取的一个组件
    graph = avfilter_graph_alloc();
    CHECK_NULL(graph);

    //根据 abuffer 获取一个音频输入端filter
    abuffersrc = avfilter_get_by_name("abuffer");
    CHECK_NULL(abuffersrc);
    //获取该filter的上下文环境
    abuffersrc_ctx = avfilter_graph_alloc_filter(graph, abuffersrc, "src");
    CHECK_NULL(abuffersrc_ctx);
    //构造参数配置（输入音频格式、输入采样率、输入声道配置） --  方式一
    //部分平台上（例如MacOS） PRIx64无法识别，需要添加编译宏定义 __STDC_FORMAT_MACROS
    snprintf(options_str, sizeof(options_str),
             "sample_fmt=%s:sample_rate=%d:channel_layout=0x%" PRIx64 ,
             av_get_sample_fmt_name(sample_format),
             sample_rate,
             sample_channel);
    //传入参数开始初始化
    ret = avfilter_init_str(abuffersrc_ctx, options_str);
    CHECK_RESULT(ret);
    //构造参数配置（输入音频格式、输入采样率、输入声道配置） --  方式二
//    av_get_channel_layout_string(ch_layout, sizeof(ch_layout), 0, sample_channel);
//    av_opt_set(abuffersrc_ctx, "channel_layout", ch_layout, AV_OPT_SEARCH_CHILDREN);
//    av_opt_set(abuffersrc_ctx, "sample_fmt", av_get_sample_fmt_name(
//            static_cast<AVSampleFormat>(sample_format)), AV_OPT_SEARCH_CHILDREN);
//    av_opt_set_int(abuffersrc_ctx, "sample_rate", sample_rate, AV_OPT_SEARCH_CHILDREN);
//    ret = avfilter_init_str(abuffersrc_ctx, NULL);
//    CHECK_RESULT(ret);

    //根据 volume 获取一个音量调节的filter
    volume = avfilter_get_by_name("volume");
    CHECK_NULL(volume);
    //获取该filter的上下文环境
    volume_ctx = avfilter_graph_alloc_filter(graph, volume, "volume");
    CHECK_NULL(volume_ctx);
    //构造参数配置（目标调节音量的大小，例如0.5为原来的一半） --  方式一
    snprintf(options_str, sizeof(options_str),
             "volume=%s", volme_value.c_str());
    //传入参数开始初始化
    ret = avfilter_init_str(volume_ctx, options_str);
    CHECK_RESULT(ret);
    //构造参数配置（目标调节音量的大小，例如0.5为原来的一半） --  方式三
//    av_dict_set(&options_dict, "volume", AV_STRINGIFY(volme_value), 0);
//    ret = avfilter_init_dict(volume_ctx, &options_dict);
//    CHECK_RESULT(ret);

    //根据 aformat 获取一个音频格式转换的filter
    aformat = avfilter_get_by_name("aformat");
    CHECK_NULL(aformat);
    //获取该filter的上下文环境
    aformat_ctx = avfilter_graph_alloc_filter(graph, aformat, "aformat");
    CHECK_NULL(aformat_ctx);
    //构造参数配置（输出音频格式、输出采样率、输出声道配置） --  方式一
    snprintf(options_str, sizeof(options_str),
             "sample_fmts=%s:sample_rates=%d:channel_layouts=0x%" PRIx64 ,
             av_get_sample_fmt_name(sample_format),
             sample_rate,
             sample_channel);
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

    //开始链接filter,这里业务逻辑的具体顺序是  abuffer -> (aformat) -> volume -> aformat -> abuffersink
    //(aformat)由于filter会默认在 abuffer -> volume 会根据输入情况插入一个格式转换filter导致输出格式变化
    //需要我们需要在最后添加一个aformat将格式转换为我们期望的输出格式
    ret = avfilter_link(abuffersrc_ctx, 0, volume_ctx, 0);
    CHECK_RESULT(ret);
    ret = avfilter_link(volume_ctx, 0, aformat_ctx, 0);
    CHECK_RESULT(ret);
    ret = avfilter_link(aformat_ctx, 0, abuffersink_ctx, 0);
    CHECK_RESULT(ret);

    //初始化整个filter链，初始化完毕
    ret = avfilter_graph_config(graph, NULL);
    CHECK_RESULT(ret);

    init_success = true;

    return 0;
}


int VolumeFilter::set_option_value(std::string key, std::string vaule)
{
    int result = -1;
    switch (hash(key.c_str())) {
        case "format"_hash:
            sample_format = strcmp(vaule.c_str(), "u8") == 0 ?  AV_SAMPLE_FMT_U8 : strcmp(vaule.c_str(), "s32") == 0 ? AV_SAMPLE_FMT_S32 : AV_SAMPLE_FMT_S16;
            result = 0;
            break;
        case "channel"_hash:
            sample_channel = strcmp(vaule.c_str(), "2") == 0 ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;
            result = 0;
            break;
        case "rate"_hash:
            sample_rate = atoi(vaule.c_str());
            result = 0;
            break;
        case "volume"_hash:
            volme_value = vaule;
            result = 0;
            break;
    }
    return result;
}

int VolumeFilter::add_frame(Frame *frame, int index)
{
    AVFrame *avframe;
    int ret;
    if (!init_success)
        return -1;
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
    ret = av_buffersrc_add_frame(abuffersrc_ctx, avframe);
    CHECK_RESULT_GOTO(ret, err);
    //我们不需要这个AVFrame了，需要释放
    av_frame_free(&avframe);
    return 0;

err:
    av_frame_free(&avframe);
    return -1;
}

VolumeFilter::~VolumeFilter()
{
    avfilter_free(abuffersrc_ctx);
    avfilter_free(abuffersink_ctx);
    avfilter_free(volume_ctx);
    avfilter_free(aformat_ctx);
    avfilter_graph_free(&graph);
}

Frame *VolumeFilter::get_frame(int index)
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
    size = av_samples_get_buffer_size(NULL, (sample_channel == AV_CH_LAYOUT_MONO ? 1 : 2), avframe->nb_samples, sample_format, 1);
    CHECK_RESULT_GOTO(read_size, err);
    frame_set_data(frame, avframe->extended_data[0], size);

    //我们不需要这个AVFrame了，需要释放
    av_frame_free(&avframe);
    return frame;

err:
    av_frame_free(&avframe);
    return nullptr;
}
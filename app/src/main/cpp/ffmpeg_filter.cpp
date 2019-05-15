
#include "ffmpeg_filter.h"
#include "hash_compare.h"
#include "volume_filter.h"
#include "amix_filter.h"
#include <string>

int Filter::set_option_value(std::string key, std::string vaule)
{
    return -1;
}

Filter::~Filter()
{

}

Filter *find_filter_by_name(std::string name)
{
    Filter *filter = NULL;
    switch (hash(name.c_str())) {
        case "volume"_hash:
            filter = new VolumeFilter();
            break;
        case "amix"_hash:
            filter = new AmixFilter();
            break;
    }
    return filter;
}

void free_filter(Filter **filter)
{
    Filter *cur = *filter;
    delete cur;
    *filter = nullptr;
}

int frame_set_data(Frame *frame, const void *data, const size_t size, const size_t index)
{
    if (!frame || index >= AV_NUM_DATA_POINTERS)
        return -1;
    if (!frame->data[index])
    {
        free(frame->data[index]);
        frame->linesize[index] = 0;
    }
    frame->data[index] = static_cast<int8_t *>(malloc(size * sizeof(int8_t)));
    if (!frame->data[index])
        return -1;
    frame->linesize[index] = size;
    memcpy(frame->data[index], data, size);
    return 0;
}

Frame *malloc_frame(DataType type)
{
    Frame *frame = new Frame();
    memset(frame, 0, sizeof(Frame));
    frame->type = type;
    return frame;
}

void free_frame(Frame **frame)
{
    Frame *cur = *frame;
    if (!cur)
        return;
    for (int i = 0; i < AV_NUM_DATA_POINTERS; i++)
    {
        if (!cur->data[i] && cur->linesize[0] != 0)
            free(cur->data[i]);
    }
    delete cur;
    *frame = nullptr;
}

static void ffmpeg_log_callback(void *ptr, int level, const char *fmt, va_list vl)
{
    static int print_prefix = 1;
    static char prev[1024];
    char line[1024];
    av_log_format_line(ptr, level, fmt, vl, line, sizeof(line), &print_prefix);
    strcpy(prev, line);
//    LOGI("%s", line);
}

void setFilterDebug(int debug)
{
    if (debug > 0)
    {
        av_log_set_level(AV_LOG_TRACE);
        av_log_set_callback(ffmpeg_log_callback);
    }
    else
    {
        av_log_set_level(AV_LOG_TRACE);
        av_log_set_callback(nullptr);
    }
}

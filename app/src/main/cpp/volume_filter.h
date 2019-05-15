
#ifndef __VOLUME_FILTER__
#define __VOLUME_FILTER__

#include "ffmpeg_filter.h"

extern "C" {
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/channel_layout.h"
};

class VolumeFilter : public Filter {

public:
    VolumeFilter();
    ~VolumeFilter();
    int set_option_value(std::string key, std::string vaule);
    int init();
    int add_frame(Frame *frame, int index = 0);
    Frame *get_frame(int index = 0);

private:
    AVFilterGraph *graph;
    AVFilterContext *abuffersrc_ctx;
    AVFilterContext *abuffersink_ctx;
    AVFilterContext *volume_ctx;
    AVFilterContext *aformat_ctx;

    int sample_rate;
    uint64_t sample_channel;
    AVSampleFormat sample_format;
    std::string volme_value;

    bool init_success;

};


#endif

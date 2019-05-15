
#ifndef __AMIX_FILTER__
#define __AMIX_FILTER__

extern "C" {
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/channel_layout.h"
};

#include "ffmpeg_filter.h"

class AmixFilter : public Filter {

public:
    AmixFilter();
    ~AmixFilter();
    int set_option_value(std::string key, std::string vaule);
    int init();
    int add_frame(Frame *frame, int index = 0);
    Frame *get_frame(int index = 0);

private:
    int sample_rate_in0, sample_rate_in1, sample_rate_out;
    uint64_t sample_channel_in0, sample_channel_in1, sample_channel_out;
    AVSampleFormat sample_format_in0, sample_format_in1, sample_format_out;

    AVFilterGraph *graph;
    AVFilterContext *abuffersrc0_ctx, *abuffersrc1_ctx;
    AVFilterContext *abuffersink_ctx;
    AVFilterContext *aformat_ctx;
    AVFilterContext *amix_ctx;

    bool init_success;
};


#endif

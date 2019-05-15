
#ifndef __FFMEPG_FILTER__
#define __FFMEPG_FILTER__

#include <string>
#include <stdint.h>
#include <cstring>
#include <cstdlib>
#include <android/log.h>

typedef enum _DataType {
    AUDIO, VIDEO
} DataType;

typedef struct _Frame {
#define AV_NUM_DATA_POINTERS 8
    int8_t *data[AV_NUM_DATA_POINTERS];
    int linesize[AV_NUM_DATA_POINTERS];
    DataType type;
} Frame;

class Filter {
public:
    virtual int set_option_value(std::string key, std::string vaule);
    virtual ~Filter();
    virtual int init() = 0;
    virtual int add_frame(Frame *frame, int index = 0) = 0;
    virtual Frame *get_frame(int index = 0) = 0;
};

Filter *find_filter_by_name(std::string name);
void free_filter(Filter **filter);
Frame *malloc_frame(DataType type);
int frame_set_data(Frame *frame, const void *data, const size_t size, const size_t index = 0);
void free_frame(Frame **frame);
void setFilterDebug(int debug);

#endif

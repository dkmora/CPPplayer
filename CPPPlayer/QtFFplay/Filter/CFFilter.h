#pragma once
#include "qtffplay_global.h"
#include "ffmpegbase.h"
#include "mediabase.h"

class QTFFPLAY_EXPORT CFFilter
{
public:
    CFFilter();
    ~CFFilter();

    RET_CODE initFilter(int width, int height, AVPixelFormat pix_fmt, AVRational time_base, AVRational sample_aspect_ratio);

    int sendFrame(AVFrame* frame);
    AVFrame* receiveFrame(int& ret);

private:
    AVFilterGraph* m_filter_graph = nullptr;
    AVFilterContext* buffersrc_ctx = nullptr;
    AVFilterContext* buffersink_ctx = nullptr;

    AVFrame* filt_frame = nullptr;

    AVFilterInOut* outputs = nullptr;
    AVFilterInOut* inputs = nullptr;
};
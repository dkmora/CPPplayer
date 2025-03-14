#include "CFFilter.h"

CFFilter::CFFilter()
{

}

CFFilter::~CFFilter()
{
    av_frame_free(&filt_frame);
    avfilter_graph_free(&m_filter_graph);
}

RET_CODE CFFilter::initFilter(int width, int height, AVPixelFormat pix_fmt, AVRational time_base, AVRational sample_aspect_ratio)
{
    // ----------------------
    // 设置过滤器图：buffer -> minterpolate -> buffersink
    // ----------------------

    int ret = 0;
    m_filter_graph = avfilter_graph_alloc();
    if (!m_filter_graph) {
        //std::cerr << "无法创建过滤器图" << std::endl;
        return RET_FAIL;
    }

    // 获取 filter：buffer（输入）和 buffersink（输出）
    const AVFilter* buffersrc = avfilter_get_by_name("buffer");
    const AVFilter* buffersink = avfilter_get_by_name("buffersink");

    char args[512];
    // 构造 buffer 参数，注意 time_base 和像素格式
    snprintf(args, sizeof(args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
         width, height, pix_fmt, time_base.num,  time_base.den,  sample_aspect_ratio.num, sample_aspect_ratio.den);

    //snprintf(args, sizeof(args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:frame_rate=%d",
    //    width, height, pix_fmt, time_base.num, time_base.den, 25);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, m_filter_graph);
    if (ret < 0) {
        //print_error("无法创建 buffer 源过滤器", ret);
        return RET_FAIL;
    }

    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, m_filter_graph);
    if (ret < 0) {
        //print_error("无法创建 buffersink 过滤器", ret);
        return RET_FAIL;
    }

    // 设置 buffersink 输出像素格式
    enum AVPixelFormat pix_fmts[] = {  pix_fmt, AV_PIX_FMT_NONE };
    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        //print_error("设置 buffersink 输出像素格式失败", ret);
        return RET_FAIL;
    }

    // 构造过滤器链描述：使用 minterpolate 插帧到 60fps
    // 参数解释：fps=60; mi_mode=mci 使用运动插值；mc_mode=aobmc 启用自适应双向运动补偿；vsbmc=1 启用垂直边缘补偿
    const char* filter_desc = "minterpolate=fps=30:mi_mode=mci:mc_mode=aobmc:me_mode=bilat";

    outputs = avfilter_inout_alloc();
    inputs = avfilter_inout_alloc();

    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    ret = avfilter_graph_parse_ptr(m_filter_graph, filter_desc, &inputs, &outputs, NULL);
    if (ret < 0) {
        //print_error("解析过滤器图描述失败", ret);
        return RET_FAIL;
    }

    ret = avfilter_graph_config(m_filter_graph, NULL);
    if (ret < 0) {
        //print_error("配置过滤器图失败", ret);
        return RET_FAIL;
    }

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    // ----------------------
    // 开始读取、解码、过滤（插帧）视频帧
    // ----------------------
    //AVPacket packet;
    //AVFrame* frame = av_frame_alloc();
    filt_frame = av_frame_alloc();
    if (!filt_frame) {
        //std::cerr << "无法分配帧" << std::endl;
        return RET_FAIL;
    }
    return RET_OK;
}

int CFFilter::sendFrame(AVFrame* frame)
{
    int ret = 0;
    ret = av_buffersrc_add_frame(buffersrc_ctx, frame);
    if(ret >= 0){
        return RET_OK;
    }
    return RET_FAIL;
}

AVFrame* CFFilter::receiveFrame(int &ret)
{
    ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
    return filt_frame;
}
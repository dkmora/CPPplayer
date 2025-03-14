extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/pixfmt.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"
#include "libavutil/log.h"
#include "libavutil/time.h"
#include "libavutil/common.h"
#include "libavutil/opt.h"
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

const char* input_file = "D:\\sound_in_sync_test.mp4";
const char* output_file = "output_60fps.mp4";

int main() {
    avformat_network_init();

    AVFormatContext* input_format_ctx = nullptr;
    AVCodecContext* decoder_ctx = nullptr;
    AVCodecContext* encoder_ctx = nullptr;
    AVFormatContext* output_format_ctx = nullptr;
    AVFilterGraph* filter_graph = avfilter_graph_alloc();
    AVFilterContext* buffersrc_ctx = nullptr, * buffersink_ctx = nullptr, * minterpolate_ctx = nullptr;
    int video_stream_index = -1;

    // 1️⃣ 打开输入文件
    if (avformat_open_input(&input_format_ctx, input_file, NULL, NULL) < 0) {
        printf("无法打开输入文件！\n");
        return -1;
    }
    if (avformat_find_stream_info(input_format_ctx, NULL) < 0) {
        printf("无法获取流信息！\n");
        return -1;
    }

    // 2️⃣ 查找视频流
    for (int i = 0; i < input_format_ctx->nb_streams; i++) {
        if (input_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1) {
        printf("没有找到视频流！\n");
        return -1;
    }

    // 3️⃣ 初始化解码器
    AVCodec* decoder = avcodec_find_decoder(input_format_ctx->streams[video_stream_index]->codecpar->codec_id);
    decoder_ctx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(decoder_ctx, input_format_ctx->streams[video_stream_index]->codecpar);
    avcodec_open2(decoder_ctx, decoder, NULL);

    // 4️⃣ 初始化滤镜 (minterpolate)
    const AVFilter* buffersrc = avfilter_get_by_name("buffer");
    const AVFilter* minterpolate = avfilter_get_by_name("minterpolate");
    const AVFilter* buffersink = avfilter_get_by_name("buffersink");

    char filter_args[512];
    snprintf(filter_args, sizeof(filter_args), "video_size=%dx%d:pix_fmt=yuv420p:time_base=1/25",
        decoder_ctx->width, decoder_ctx->height);

    avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", filter_args, NULL, filter_graph);
    avfilter_graph_create_filter(&minterpolate_ctx, minterpolate, "interp", "fps=30:mi_mode=mci:mc_mode=aobmc:me_mode=bilat", NULL, filter_graph);
    avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph);
    avfilter_link(buffersrc_ctx, 0, minterpolate_ctx, 0);
    avfilter_link(minterpolate_ctx, 0, buffersink_ctx, 0);
    avfilter_graph_config(filter_graph, NULL);

    // 5️⃣ 初始化编码器
    AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    encoder_ctx = avcodec_alloc_context3(encoder);
    encoder_ctx->bit_rate = decoder_ctx->bit_rate;
    encoder_ctx->width = decoder_ctx->width;
    encoder_ctx->height = decoder_ctx->height;
    encoder_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    encoder_ctx->time_base = { 1, 30 }; // 60fps
    avcodec_open2(encoder_ctx, encoder, NULL);

    // 6️⃣ 创建输出 MP4
    avformat_alloc_output_context2(&output_format_ctx, NULL, NULL, output_file);
    AVStream* out_stream = avformat_new_stream(output_format_ctx, encoder);
    avcodec_parameters_from_context(out_stream->codecpar, encoder_ctx);
    avio_open(&output_format_ctx->pb, output_file, AVIO_FLAG_WRITE);
    avformat_write_header(output_format_ctx, NULL);

    bool _stop = false;
    // 7️⃣ 读取帧 → 插帧 → 编码 → 写入 MP4
    AVPacket packet;
    AVFrame* frame = av_frame_alloc();
    while (av_read_frame(input_format_ctx, &packet) >= 0) {
        if (packet.stream_index == video_stream_index) {
            if (_stop)
                break;

            avcodec_send_packet(decoder_ctx, &packet);
            while (avcodec_receive_frame(decoder_ctx, frame) == 0) {
                av_buffersrc_add_frame(buffersrc_ctx, frame);
                AVFrame* filt_frame = av_frame_alloc();
                while (av_buffersink_get_frame(buffersink_ctx, filt_frame) == 0) {
                    avcodec_send_frame(encoder_ctx, filt_frame);
                    AVPacket out_pkt;
                    av_init_packet(&out_pkt);
                    if (avcodec_receive_packet(encoder_ctx, &out_pkt) == 0) {
                        out_pkt.stream_index = out_stream->index;
                        av_interleaved_write_frame(output_format_ctx, &out_pkt);
                    }
                    av_packet_unref(&out_pkt);
                    av_frame_unref(filt_frame);
                }
                av_frame_free(&filt_frame);
            }
        }
        av_packet_unref(&packet);
    }

    // 8️⃣ 关闭并释放资源
    av_write_trailer(output_format_ctx);
    avformat_close_input(&input_format_ctx);
    avcodec_free_context(&decoder_ctx);
    avcodec_free_context(&encoder_ctx);
    avformat_free_context(output_format_ctx);
    avfilter_graph_free(&filter_graph);
    av_frame_free(&frame);

    printf("插帧完成，生成 60FPS MP4!\n");
    return 0;
}

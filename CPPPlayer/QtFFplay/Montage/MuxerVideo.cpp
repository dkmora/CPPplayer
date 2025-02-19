#include "MuxerVideo.h"

MuxerVideo::MuxerVideo()
{

}

MuxerVideo::~MuxerVideo()
{
}


bool MuxerVideo::startMuxer(std::string _file_name, int _width, int _hright, int _fps, int _sample_rate, int _channels)
{
    m_fmt_ctx = nullptr;
    if (avformat_alloc_output_context2(&m_fmt_ctx, nullptr, "mp4", _file_name.c_str()) < 0) {
        std::cerr << "Could not create output context\n";
        return -1;
    }

    // 添加视频流
    m_video_stream = add_video_stream(m_fmt_ctx, AV_CODEC_ID_H264, _width, _hright, _fps);
    if (!m_video_stream) {
        std::cerr << "Failed to create video stream\n";
        return -1;
    }

    // 添加音频流
    m_audio_stream = add_audio_stream(m_fmt_ctx, AV_CODEC_ID_AAC, _sample_rate, _channels);
    if (!m_audio_stream) {
        std::cerr << "Failed to create audio stream\n";
        return -1;
    }

    // 打开输出文件
    if (!(m_fmt_ctx->flags & AVFMT_NOFILE)) {
        if (avio_open(&m_fmt_ctx->pb, _file_name.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Could not open output file\n";
            return -1;
        }
    }

    // 写文件头
    if (avformat_write_header(m_fmt_ctx, nullptr) < 0) {
        std::cerr << "Error occurred when writing header\n";
        return -1;
    }
}

void MuxerVideo::pushYUV(AVFrame* frame, double pts)
{
    write_video_frames(m_fmt_ctx, m_video_stream, m_video_stream->codec, frame, 1, pts);
}

void MuxerVideo::pushPCM(AVFrame* frame, double pts)
{
    write_audio_frames(m_fmt_ctx, m_audio_stream, m_audio_stream->codec, frame, 1, pts);
}

void MuxerVideo::endMuxer()
{   
	// 写文件尾部
	av_write_trailer(m_fmt_ctx);

	// 清理资源
	if (!(m_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
		avio_close(m_fmt_ctx->pb);
	}
	avformat_free_context(m_fmt_ctx);
	return;
}

// 初始化视频流
AVStream* MuxerVideo::add_video_stream(AVFormatContext* fmt_ctx, AVCodecID codec_id, int width, int height, int fps) {
    AVCodec* codec = avcodec_find_encoder(codec_id);
    if (!codec) {
        std::cerr << "Video codec not found\n";
        return nullptr;
    }

    AVStream* stream = avformat_new_stream(fmt_ctx, codec);
    if (!stream) {
        std::cerr << "Could not allocate video stream\n";
        return nullptr;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        std::cerr << "Could not allocate video codec context\n";
        return nullptr;
    }

    //stream->codecpar->codec_id = codec_id;
    //stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    //stream->codecpar->width = width;
    //stream->codecpar->height = height;
    //stream->codecpar->format = AV_PIX_FMT_YUV420P;
    //stream->time_base = { 1, fps };

    codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    codec_ctx->bit_rate = 500 * 1024;
    codec_ctx->width = width;
    codec_ctx->height = height;
    codec_ctx->framerate = { fps, 1 };
    // 视频的 如果不主动设置：The encoder timebase is not set
    codec_ctx->time_base = { 1, 1000000 };   // 单位为微妙

    codec_ctx->gop_size = fps;
    codec_ctx->max_b_frames = 0;
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    //    av_dict_set(&dict_, "tune", "zerolatency", 0);

    //if (avcodec_parameters_from_context(stream->codecpar, codec_ctx) < 0) {
    //   std::cerr << "Failed to copy codec parameters\n";
    //    return nullptr;
    //}

    if (avcodec_open2(codec_ctx, codec, &dict_) < 0) {
        std::cerr << "Could not open codec\n";
        return nullptr;
    }

    stream->codec = codec_ctx;
    return stream;
}

// 初始化音频流
AVStream* MuxerVideo::add_audio_stream(AVFormatContext* fmt_ctx, AVCodecID codec_id, int sample_rate, int channels) {
    AVCodec* codec = avcodec_find_encoder(codec_id);
    if (!codec) {
        std::cerr << "Audio codec not found\n";
        return nullptr;
    }

    AVStream* stream = avformat_new_stream(fmt_ctx, codec);
    if (!stream) {
        std::cerr << "Could not allocate audio stream\n";
        return nullptr;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        std::cerr << "Could not allocate audio codec context\n";
        return nullptr;
    }

    //stream->codecpar->codec_id = codec_id;
    //stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    //stream->codecpar->sample_rate = sample_rate;
    //stream->codecpar->channels = channels;
    //stream->codecpar->channel_layout = av_get_default_channel_layout(channels);
    //stream->codecpar->format = AV_SAMPLE_FMT_FLTP;
    //stream->codecpar->frame_size = 1024;
    //stream->time_base = { 1, sample_rate };

    codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    codec_ctx->bit_rate = 500 * 1024;
    codec_ctx->sample_rate = sample_rate;
    codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    codec_ctx->channels = channels;
    codec_ctx->channel_layout = av_get_default_channel_layout(codec_ctx->channels);
    codec_ctx->frame_size = 1024;

    //if (avcodec_parameters_from_context(stream->codecpar, codec_ctx) < 0) {
    //    std::cerr << "Failed to copy codec parameters\n";
    //    return nullptr;
    //}

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        std::cerr << "Could not open codec\n";
        return nullptr;
    }

    stream->codec = codec_ctx;
    return stream;
}


void MuxerVideo::write_video_frames(AVFormatContext* fmt_ctx, AVStream* video_stream, AVCodecContext* video_codec_ctx, AVFrame* frame, int frame_count, double pts) {
    AVFrame* dst_frame = av_frame_alloc();
    dst_frame->format = video_codec_ctx->pix_fmt;
    dst_frame->width = video_codec_ctx->width;
    dst_frame->height = video_codec_ctx->height;
    dst_frame->linesize[0] = frame->linesize[0];
    dst_frame->linesize[1] = frame->linesize[1];
    dst_frame->linesize[2] = frame->linesize[2];
    if (av_frame_get_buffer(dst_frame, 0) < 0) {
        fprintf(stderr, "Failed to allocate buffer for dst frame\n");
        return ;
    }

    for (int i = 0; i < frame_count; ++i) {
        if (av_frame_copy(dst_frame, frame) < 0) {
            av_frame_free(&dst_frame);
            continue;
        }

        pts = av_rescale_q(pts, AVRational{ 1, (int)1000000 }, video_codec_ctx->time_base);
        dst_frame->pts = pts;

        // 发送帧到编码器
        int ret = avcodec_send_frame(video_codec_ctx, dst_frame);
        if (ret < 0) {
            std::cerr << "Error sending video frame for encoding\n";
            continue;
        }

        while (1)
        {
            AVPacket* packet = av_packet_alloc();
            ret = avcodec_receive_packet(video_codec_ctx, packet);
            packet->stream_index = video_stream->index;
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                ret = 0;
                av_packet_free(&packet);
                break;
            }
            else if (ret < 0) {
                char errbuf[1024] = { 0 };
                av_strerror(ret, errbuf, sizeof(errbuf) - 1);
                printf("h264 avcodec_receive_packet failed:%s\n", errbuf);
                av_packet_free(&packet);
                ret = -1;
            }
            packet->stream_index = video_stream->index;

            AVRational src_time_base;   // 编码后的包
            AVRational dst_time_base;   // mp4输出文件对应流的time_base
            src_time_base = video_codec_ctx->time_base;
            dst_time_base = video_stream->time_base;

            // 时间基转换
            packet->pts = av_rescale_q(packet->pts, src_time_base, dst_time_base);
            packet->dts = av_rescale_q(packet->dts, src_time_base, dst_time_base);
            packet->duration = av_rescale_q(packet->duration, src_time_base, dst_time_base);

            av_interleaved_write_frame(fmt_ctx, packet);
            av_packet_unref(packet);
            printf("h264 pts:%lld\n", packet->pts);
        }
    }
    av_frame_free(&dst_frame);
}

void MuxerVideo::write_audio_frames(AVFormatContext* fmt_ctx, AVStream* audio_stream, AVCodecContext* audio_codec_ctx, AVFrame* frame, int sample_count, double pts) {
    for (int i = 0; i < sample_count; ++i) {
        pts = av_rescale_q(pts, AVRational{ 1, (int)1000000 }, audio_codec_ctx->time_base);
        frame->pts = pts;

        int ret = avcodec_send_frame(audio_codec_ctx, frame);
        if (ret != 0) {
            char errbuf[1024] = { 0 };
            av_strerror(ret, errbuf, sizeof(errbuf) - 1);
            printf("avcodec_send_frame failed:%s\n", errbuf);
            return;
        }
        while (1)
        {
            AVPacket* packet = av_packet_alloc();
            ret = avcodec_receive_packet(audio_codec_ctx, packet);
            packet->stream_index = audio_stream->index;
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                ret = 0;
                av_packet_free(&packet);
                break;
            }
            else if (ret < 0) {
                char errbuf[1024] = { 0 };
                av_strerror(ret, errbuf, sizeof(errbuf) - 1);
                printf("aac avcodec_receive_packet failed:%s\n", errbuf);
                av_packet_free(&packet);
                ret = -1;
            }

            AVRational src_time_base;   // 编码后的包
            AVRational dst_time_base;   // mp4输出文件对应流的time_base
            src_time_base = audio_codec_ctx->time_base;
            dst_time_base = audio_stream->time_base;

            // 时间基转换
            packet->pts = av_rescale_q(packet->pts, src_time_base, dst_time_base);
            packet->dts = av_rescale_q(packet->dts, src_time_base, dst_time_base);
            packet->duration = av_rescale_q(packet->duration, src_time_base, dst_time_base);

            av_interleaved_write_frame(fmt_ctx, packet);
            av_packet_unref(packet);
        }
    }
    av_frame_free(&frame);
}

#pragma once
#include "ffmpegbase.h"
#include <iostream>
#include "qtffplay_global.h"

class QTFFPLAY_EXPORT MuxerVideo
{
public:
    MuxerVideo();
    ~MuxerVideo();

    bool startMuxer(std::string _file_name, int _width, int _hright, int _fps, int _sample_rate = 44100, int _channels = 2);

    void pushYUV(AVFrame* frame, double pts);

    void pushPCM(AVFrame* frame, double pts);

    void endMuxer();

    void flush_encoder(AVCodecContext* enc_ctx, AVFormatContext* fmt_ctx, AVStream* stream);

private:
    AVStream* add_video_stream(AVFormatContext* fmt_ctx, AVCodecID codec_id, int width, int height, int fps);
    AVStream* add_audio_stream(AVFormatContext* fmt_ctx, AVCodecID codec_id, int sample_rate, int channels);

    void write_video_frames(AVFormatContext* fmt_ctx, AVStream* video_stream, AVCodecContext* video_codec_ctx, AVFrame* frame, int frame_count, double pts);
    void write_audio_frames(AVFormatContext* fmt_ctx, AVStream* audio_stream, AVCodecContext* audio_codec_ctx, AVFrame* frame, int sample_count, double pts);

private:
    std::string m_file_name;
    AVFormatContext* m_fmt_ctx = nullptr;
    AVStream* m_video_stream = nullptr;
    AVStream* m_audio_stream = nullptr;
    AVDictionary* dict_ = NULL;
};
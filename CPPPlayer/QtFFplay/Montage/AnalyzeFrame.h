#pragma once
#include "qtffplay_global.h"
#include "ffmpegbase.h"
#include "mediabase.h"
#include "AVDecoder.h"

#include <list>
#include <queue>
#include <iostream>

class QTFFPLAY_EXPORT AnalyzeFrameEngine
{
public:
    AnalyzeFrameEngine();
    ~AnalyzeFrameEngine();

    /*
     * @brief 开始解析视频
     */
    int startAnalyze(std::string file_name);

    /*
     * @brief 清空缓存视频帧
     */
    void clearFrame();

    /*
     * @brief 获取文件的总时长
     */
    int64_t get_file_duration() { if (m_avformat_context == nullptr) return 0; else return m_avformat_context->duration; }

    /*
     * @brief 获取文件名
     */
    std::string get_file_name();

    /*
     * @brief 开始解码
     */
    void startDecode(int width, int height);

    /*
     * @brief 获取I帧
     */
    std::list<AVFrame*> getIFrameList()  { return m_IFrame; };

    /*
     * @brief 获取宽高帧率
     */
    int get_video_width() { return m_avformat_context->streams[m_video_stream]->codecpar->width;}
    int get_video_height() { return m_avformat_context->streams[m_video_stream]->codecpar->height; };
    int get_video_fps() { return av_q2d(m_avformat_context->streams[m_video_stream]->r_frame_rate); };

    /*
     * @brief 获取一帧音视频解码后的数据
     */
    AVFrame* getVidioDecode();
    AVFrame* getAudioDecode();

    bool getIsPlay() { return m_isDone; }

private:
    void read_thread();

    RET_CODE getPicture();

    AVStream* add_video_stream(AVFormatContext* fmt_ctx, AVCodecID codec_id, int width, int height, int fps);
    AVStream* add_audio_stream(AVFormatContext* fmt_ctx, AVCodecID codec_id, int sample_rate, int channels);

    RET_CODE allocation_decoder(FFDecoder* coder, int stream);
    RET_CODE release_decoder(FFDecoder* coder);

private:
    std::string m_file_name;

    int m_audio_stream = -1;
    int m_video_stream = -1;
    int m_fps = 0;
    int m_video_index = 0;
    int  m_eof = 0;        // 是否读取结束

    // av_read_frame 获取的avpacket
    AVPacket* m_avpacket = nullptr;

    AVFormatContext* m_avformat_context = nullptr;

    AVPacketQueue m_avpacket_queue; // 音视频队列

    FFDecoder m_video_decoder; // 视频解码器
    FFDecoder m_audio_decoder; // 音频解码器

    AVClock m_av_clock; // 时钟

    cvpublish::AVDecoder* m_video_decode_thread = NULL; // 视频解码线程
    cvpublish::AVDecoder* m_audio_decode_thread = NULL; // 音频解码线程

    std::list<AVFrame*> m_IFrame;

    bool m_isDone = false;
    std::thread* m_read_thread = nullptr;  // 读取数据线程
};
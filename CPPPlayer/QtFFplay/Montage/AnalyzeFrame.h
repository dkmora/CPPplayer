#pragma once
#include "qtffplay_global.h"
#include "ffmpegbase.h"
#include "mediabase.h"
#include "AVDecoder.h"
#include "MediaPlayerEvent.h"

#include <list>
#include <queue>
#include <iostream>
#include <future>

#define ONPLAYERSTATECHANGED_EVENT(state, error) setPlayerStateChanged(state, error);

typedef std::function<void(void)> StartplayCallBack;

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
     * @brief 获取文件的总时长 单位微妙
     */
    int64_t get_file_duration() { if (m_avformat_context == nullptr) return 0; else return m_avformat_context->duration; }

    /*
     * @brief 获取文件名
     */
    std::string get_file_name();

    /*
     * @brief 开始解码
     */
    void startDecode(int width = 0, int height = 0);

    /*
     * @brief 播放
     */
    void play(StartplayCallBack cb);

    /*
     *  @brief 暂停
     */
    void Pause();

    /**
     * @brief seek in the stream
     * @param pos  具体seek到的位置
     * @param rel  增量情况
     * @param seek_by_bytes
     */
    void Seek(int64_t pos, int64_t rel, int seek_by_bytes);

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
    AVFrame* getVideoDecodeFrame();
    AVFrame* getAudioDecodeFrame();

    /*
     * @beief 获取视频解码器
    */
    FFDecoder* getVideoDecoder() { return &m_video_decoder; }

    /*
    * @beief 获取音频解码器
    */
    FFDecoder* getAudioDecoder() { return &m_audio_decoder; }

    /*
     * @brief 获取音视频解码线程
     */
    cvpublish::AVDecoder* getVidioDecode();
    cvpublish::AVDecoder* getAudioDecode();

    /*
     * @brief 设置裁剪开始时间 单位秒
     */
    void setStartTime(int64_t startTime) { m_start_time = startTime; }
    int64_t getStartTime() { return m_start_time; }

    /*
     * @brief 设置裁剪结束时间 单位秒
     */
    void setEndTime(int64_t endtime) { m_end_time = endtime; }
    int64_t getEndTime() { return m_end_time; }

    bool getIsPlay() { return m_isDone; }

    void setMuxerIndex(int index) { m_muxer_index = index; }

    /*
    *  设置播放器事件回调指针
    */
    void setMediaPlayerEventHandler(MediaPlayerEventHandler* eventHandler) { m_mediaplayerEventHandler = eventHandler; }

private:
    void read_thread();

    RET_CODE getPicture();

    AVStream* add_video_stream(AVFormatContext* fmt_ctx, AVCodecID codec_id, int width, int height, int fps);
    AVStream* add_audio_stream(AVFormatContext* fmt_ctx, AVCodecID codec_id, int sample_rate, int channels);

    RET_CODE allocation_decoder(FFDecoder* coder, int stream);
    RET_CODE release_decoder(FFDecoder* coder);
    // 返回状态码
    void setPlayerStateChanged(MediaPlayerState state, MediaPlayerError error);

private:
    std::string m_file_name;

    StartplayCallBack m_startplay_callback = nullptr;

    int m_audio_stream = -1;
    int m_video_stream = -1;
    int m_fps = 0;
    int m_video_index = 0;
    int  m_eof = 0;        // 是否读取结束

    // seek
    int	    m_seek_req = 0;    // 标识一次seek请求
    int	    m_seek_flags = AVSEEK_FLAG_BYTE;  // seek标志，诸如AVSEEK_FLAG_BYTE等
    int64_t	m_seek_pos = 0;    // 请求seek的目标位置(当前位置+增量)
    int64_t	m_seek_rel = 0;    // 本次seek的位置增量

    MediaPlayerEventHandler* m_mediaplayerEventHandler = nullptr;

    AVPacket* m_avpacket = nullptr; // av_read_frame 获取的avpacket

    AVFormatContext* m_avformat_context = nullptr;

    AVPacketQueue m_avpacket_queue; // 音视频队列

    FFDecoder m_video_decoder; // 视频解码器
    FFDecoder m_audio_decoder; // 音频解码器

    AVClock m_av_clock; // 时钟

    cvpublish::AVDecoder* m_video_decode_thread = NULL; // 视频解码线程
    cvpublish::AVDecoder* m_audio_decode_thread = NULL; // 音频解码线程

    std::list<AVFrame*> m_IFrame;

    std::thread* m_async_thread = nullptr; // 异步操作线程
    std::thread* m_read_thread = nullptr;  // 读取数据线程
    std::mutex* m_wait_mutex = nullptr;    // 读取数据锁
    std::condition_variable* m_cond_t_read_thread = nullptr; // 唤醒读取数据线程条件变量

    bool m_isDone = false;
    bool m_paused = false; // 暂停

    bool m_bReadFrame = true; // 打断堵塞，例如av_read_frame堵塞
    int m_timeoutReadFrame = 0; // 打断堵塞，例如av_read_frame堵塞
    uint64_t m_readframe_callback_time = 0;

    int m_muxer_index = 0; //视频合并排序索引

    // 裁剪视频起始结束位置-单位毫秒
    uint64_t m_start_time = 0;
    uint64_t m_end_time = 0; 

    // 累计时长
    uint64_t m_video_total_time = 0;
    uint64_t m_audio_total_time = 0;
};
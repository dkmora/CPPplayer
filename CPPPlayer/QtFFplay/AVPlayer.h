#pragma once

/*
* QtFFplay
* 主要功能是做解复用，从码流中分离音视频packet，并插入缓存队列
*/
#include "qtffplay_global.h"
#include "ffmpegbase.h"
#include "mediabase.h"
#include "AVDecoder.h"
#include "MediaPlayerEvent.h"
#include <thread>
#include <future>

#define ONPLAYERSTATECHANGED_EVENT(state, error) setPlayerStateChanged(state, error);

typedef std::function<void(void)> StartplayCallBack;

typedef enum MEDIA_COMMAND{
	MEDIA_IDLE = 0, // 默认状态
	MEDIA_START,    // 开始播放
	MEDIA_STOP,     // 停止播放
	MEDIA_PAUSE     // 暂停播放
}MEDIA_COMMAND;

class AVPlayer {
public:
	AVPlayer();
	~AVPlayer();

	/*
	* @brief 打开媒体文件 回调状态码
	*/
	RET_CODE play(std::string filename, StartplayCallBack cb);

	/*
	* @brief 关闭媒体文件 回调状态码
	*/
	void stop();

	/*
	*  @brief 暂停
	*/
	void Pause();

	//libsonic 变速变调播放 或 libsoundtouch 
	/**
	* @brief seek in the stream
    * @param pos  具体seek到的位置
    * @param rel  增量情况
    * @param seek_by_bytes
	*/
	void Seek(int64_t pos, int64_t rel, int seek_by_bytes);

	/*
	* @brief 变速播放 变速不变调（使用sonic）
	*/
	void Rate(double speed);

	/*
	* @brief 获取文件的总时长
	*/
	int64_t get_file_duration() { if (m_avformat_context == nullptr) return 0; else return m_avformat_context->duration; }

	/*
	* @brief 获取第一帧的位置
	*/
	int64_t get_file_starttime() { return m_avformat_context->start_time; }

	/*
	* @brief 获取整个文件的字节
	*/
	uint64_t get_avio_size() { return avio_size(m_avformat_context->pb); }

	/*
	* @brief 获取文件已播放时长
	*/
	double get_master_clock() { return m_av_clock.get_master_clock(); }

	/*
    * @beief 获取视频帧率
    */
	int getFps() { return m_fps; }

	/*
	* @beief 是否有视频流 true: 有 false: 没有
	*/
	bool IsHaveVideoStream() { return (m_video_stream != -1); }

	/*
	* @beief 是否有音频流 true: 有 false: 没有
	*/
	bool IsHaveAudioStream() { return (m_audio_stream != -1); }

	/*
	* @beief 获取视频解码器
	*/
	FFDecoder* getVideoDecoder() { return &m_video_decoder; }

	/*
	* @beief 获取音频解码器
	*/
	FFDecoder* getAudioDecoder() { return &m_audio_decoder; }

	cvpublish::AVDecoder* getVidioDecode();
	cvpublish::AVDecoder* getAudioDecode();

	/*
	*  设置播放器事件回调指针
	*/
	void setMediaPlayerEventHandler(MediaPlayerEventHandler* eventHandler) { m_mediaplayerEventHandler = eventHandler; }

private:
	// 播放器指令
	void mediaCommand();
	// 开始播放
	RET_CODE startPlay();
	// 结束播放
	void stopPlay();
	// 读取数据线程
	void read_thread();
	// 分配解码器
	RET_CODE allocation_decoder(FFDecoder* coder, int stream);
	// 释放解码器
	RET_CODE release_decoder(FFDecoder* coder);
	// 打断阻塞
	static int interrupt_cb(void *ctx);
	// 返回状态码
	void setPlayerStateChanged(MediaPlayerState state, MediaPlayerError error);
	//  
	void Release();
private:
	std::string m_file_name;
	StartplayCallBack m_startplay_callback = nullptr;

	//video param 需要做封装
	int m_video_stream = -1;
	int m_fps = 0;
	int m_video_index = 0;

	// audio param 需要做封装
	int m_audio_stream = -1;

	// av_read_frame 获取的avpacket
	AVPacket* m_avpacket = nullptr;

	std::thread* m_async_thread = nullptr; // 异步操作线程
	std::thread* m_read_thread = nullptr;  // 读取数据线程
	std::mutex* m_wait_mutex = nullptr;    // 读取数据锁
	std::condition_variable* m_cond_t_read_thread = nullptr; // 唤醒读取数据线程条件变量

	// 视频操作
	bool m_isplay = false;
	bool m_paused = false; // 暂停
	bool m_playInit = false;
	int  m_eof = 0;        // 是否读取结束

	// seek
	int	    m_seek_req = 0;    // 标识一次seek请求
	int	    m_seek_flags = AVSEEK_FLAG_BYTE;  // seek标志，诸如AVSEEK_FLAG_BYTE等
	int64_t	m_seek_pos = 0;    // 请求seek的目标位置(当前位置+增量)
	int64_t	m_seek_rel = 0;    // 本次seek的位置增量

	AVFormatContext* m_avformat_context = nullptr;   // iformat的上下文

	FFDecoder m_video_decoder; // 视频解码器
	FFDecoder m_audio_decoder; // 音频解码器

	AVPacketQueue m_avpacket_queue; // 音视频队列

	cvpublish::AVDecoder* m_video_decode_thread = NULL; // 视频解码线程
	cvpublish::AVDecoder* m_audio_decode_thread = NULL; // 音频解码线程

	AVClock m_av_clock; // 时钟

	bool m_bReadFrame = true; // 打断堵塞，例如av_read_frame堵塞
	int m_timeoutReadFrame = 0; // 打断堵塞，例如av_read_frame堵塞
	uint64_t m_readframe_callback_time = 0;

	MediaPlayerEventHandler* m_mediaplayerEventHandler = nullptr;
	MEDIA_COMMAND m_media_command = MEDIA_IDLE;
	std::mutex* m_command_mutex_t = nullptr;
	std::condition_variable* m_command_cond_t = nullptr;
	bool m_blruncommand = false;
};
#pragma once

/*
*   FFmpeg 音视频解码器类
*/

#include <iostream>
#include "commonlooper.h"
#include "ffmpegbase.h"
#include "mediabase.h"
#include "MyAudioResample.h"
#include "ffmpegbase.h"
#include "sonic.h"

typedef unsigned char uchar;

namespace cvpublish {

class AVDecoder : public CommonLooper {
public:

	AVDecoder(FFDecoder* ffmpegdecoder, AVPacketQueue* avpacketQueue, AVClock* videostate);
	virtual ~AVDecoder();

	// 暂停
	void Pause();
	// 获取一帧yuv视频数据
	int getVideoYuv420Frame(uint8_t** y, uint8_t** u, uint8_t** v, int& width, int& height);
	// 获取一帧pcm音频数据
	int getAudioFrame(uint8_t** data, size_t size);
	// 设置是否需要修改变速速率
	void setNeedChangeRate(bool blchange) { m_need_change_rate = blchange; }
	// 设置变速速率
	void setPlaybackRate(float rate);
	// 获取音频重采样参数
	audio_resampler_params_t* get_resampler_params() { return &m_resampler_params; }

private:
	virtual void Loop();
	int  decoder_frame();
	void Clear();

	// 设置音频时钟
	void video_set_clock_at(int write_buf_size);
	// 获取一帧音频
	int get_audio_decode_frame();
	// 计算上一帧需要持续的duration，这里有校正算法
	double vp_duration(double max_frame_duration, Frame *vp, Frame *nextvp);
	/**
     * @brief 计算正在显示帧需要持续播放的时间。
     * @param delay 该参数实际传递的是当前显示帧和待播放帧的间隔。
	 * @param clk 时钟
     * @return 返回当前显示帧要持续播放的时间。为什么要调整返回的delay？为什么不支持使用相邻间隔帧时间？
     */
	double compute_target_delay(double delay, AVClock& avclock);

	int is_normal_playback_rate();

private:
	bool m_paused = false;

	// video
	int m_video_stream = -1;
	int m_fps = 0;
	int m_video_index = 0;

	FFDecoder* m_decoder;           // 解码器
	AVPacketQueue* m_avpacketqueue; // 音视频队列
	AVClock* m_avclock;             // 音视频时钟

	AVFrame* m_av_frame;            // 音视频解码帧

	// audio
	int m_audio_stream = -1;
	int	m_rate;                   // 采样率
	int m_channels;               // 通道数
	int64_t	m_channel_layout;     // 通道布局，比如2.1声道，5.1声道等
	enum AVSampleFormat	m_fmt;    // 音频采样格式，比如AV_SAMPLE_FMT_S16表示为有符号16bit深度，交错排列模式。
	int	m_frame_size;             // 一个采样单元占用的字节数（比如2通道时，则左右通道各采样一次合成一个采样单元）
	int	m_bytes_per_sec;          // 一秒时间的字节数，比如采样率48Khz，2 channel，16bit，则一秒48000*2*16/8=192000
	int m_audio_index = 0;

	// 视频格式转换
	uint8_t* m_out_buffer = nullptr;
	AVFrame* m_avframe_yuv420 = nullptr;
	SwsContext* m_img_convert_ctx = nullptr;  

	// 音频重采样结构
	audio_resampler_params_t m_resampler_params;
	AudioResample m_audioresample; // 音频重采样器
	int m_codec_type = 0;

	int m_dst_bufsize = 0;           // 重采样后得到的音频数据
	int64_t m_audio_callback_time;   // 每次回调的时间
	double m_frame_timer = 0.0f;     // 记录最后一帧播放的时刻
	double m_frame_drops_late = 0;       // 丢弃视频frame计数
	double m_remaining_time = 0.0;     /* 休眠等待，remaining_time的计算在video_refresh中 */

	// 变速相关
	bool m_need_change_rate = false;
	float m_pf_playback_rate = 1;
	sonicStreamStruct* m_audio_speed_convert = nullptr;
	uint8_t* m_audio_speed_buf = nullptr;
	unsigned int m_audio_speed_size = 0;
	unsigned int m_audio_speed1_size = 0;
	int m_audio_buffer_index = 0;
	int m_write_buf_size = 0;

	uint8_t*     m_audio_buffer = nullptr;
	uint8_t*     m_audio_buffer1 = nullptr;
	unsigned int m_audio_buf1_size = 0;

};

}
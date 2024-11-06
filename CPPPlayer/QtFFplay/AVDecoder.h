#pragma once

/*
*   FFmpeg ����Ƶ��������
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

	// ��ͣ
	void Pause();
	// ��ȡһ֡yuv��Ƶ����
	int getVideoYuv420Frame(uint8_t** y, uint8_t** u, uint8_t** v, int& width, int& height);
	// ��ȡһ֡pcm��Ƶ����
	int getAudioFrame(uint8_t** data, size_t size);
	// �����Ƿ���Ҫ�޸ı�������
	void setNeedChangeRate(bool blchange) { m_need_change_rate = blchange; }
	// ���ñ�������
	void setPlaybackRate(float rate);
	// ��ȡ��Ƶ�ز�������
	audio_resampler_params_t* get_resampler_params() { return &m_resampler_params; }

private:
	virtual void Loop();
	int  decoder_frame();
	void Clear();

	// ������Ƶʱ��
	void video_set_clock_at(int write_buf_size);
	// ��ȡһ֡��Ƶ
	int get_audio_decode_frame();
	// ������һ֡��Ҫ������duration��������У���㷨
	double vp_duration(double max_frame_duration, Frame *vp, Frame *nextvp);
	/**
     * @brief ����������ʾ֡��Ҫ�������ŵ�ʱ�䡣
     * @param delay �ò���ʵ�ʴ��ݵ��ǵ�ǰ��ʾ֡�ʹ�����֡�ļ����
	 * @param clk ʱ��
     * @return ���ص�ǰ��ʾ֡Ҫ�������ŵ�ʱ�䡣ΪʲôҪ�������ص�delay��Ϊʲô��֧��ʹ�����ڼ��֡ʱ�䣿
     */
	double compute_target_delay(double delay, AVClock& avclock);

	int is_normal_playback_rate();

private:
	bool m_paused = false;

	// video
	int m_video_stream = -1;
	int m_fps = 0;
	int m_video_index = 0;

	FFDecoder* m_decoder;           // ������
	AVPacketQueue* m_avpacketqueue; // ����Ƶ����
	AVClock* m_avclock;             // ����Ƶʱ��

	AVFrame* m_av_frame;            // ����Ƶ����֡

	// audio
	int m_audio_stream = -1;
	int	m_rate;                   // ������
	int m_channels;               // ͨ����
	int64_t	m_channel_layout;     // ͨ�����֣�����2.1������5.1������
	enum AVSampleFormat	m_fmt;    // ��Ƶ������ʽ������AV_SAMPLE_FMT_S16��ʾΪ�з���16bit��ȣ���������ģʽ��
	int	m_frame_size;             // һ��������Ԫռ�õ��ֽ���������2ͨ��ʱ��������ͨ��������һ�κϳ�һ��������Ԫ��
	int	m_bytes_per_sec;          // һ��ʱ����ֽ��������������48Khz��2 channel��16bit����һ��48000*2*16/8=192000
	int m_audio_index = 0;

	// ��Ƶ��ʽת��
	uint8_t* m_out_buffer = nullptr;
	AVFrame* m_avframe_yuv420 = nullptr;
	SwsContext* m_img_convert_ctx = nullptr;  

	// ��Ƶ�ز����ṹ
	audio_resampler_params_t m_resampler_params;
	AudioResample m_audioresample; // ��Ƶ�ز�����
	int m_codec_type = 0;

	int m_dst_bufsize = 0;           // �ز�����õ�����Ƶ����
	int64_t m_audio_callback_time;   // ÿ�λص���ʱ��
	double m_frame_timer = 0.0f;     // ��¼���һ֡���ŵ�ʱ��
	double m_frame_drops_late = 0;       // ������Ƶframe����
	double m_remaining_time = 0.0;     /* ���ߵȴ���remaining_time�ļ�����video_refresh�� */

	// �������
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
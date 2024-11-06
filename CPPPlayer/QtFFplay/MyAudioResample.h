#pragma once

/*
* ffmpeg 音频重采样封装
* 作者：dk
*/

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/mem.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/opt.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
#include "libavutil/error.h"
#include "libavutil/frame.h"
#include "libavcodec/avcodec.h"
}

#include <iostream>

// 重采样的参数
typedef struct audio_resampler_params {
	// 输入参数
	enum AVSampleFormat src_sample_fmt;  // 输入格式
	int src_sample_rate = 0;             // 输入采样率
	uint64_t src_channel_layout = 0;     // 输入通道数
	int src_nb_channels = 0;             // 布局通道数
	int src_linesize = 0;                // 缓冲区大小 (采样点*通道数*采样位数) 采样位数和音频格式有关
	int src_nb_samples = 0;              // 采样点
	uint8_t **src_data = NULL;           // 数据缓冲区

	// 输出参数
	enum AVSampleFormat dst_sample_fmt;  // 输出格式
	int dst_sample_rate = 0;             // 输出采样率
	uint64_t dst_channel_layout = 0;     // 输出通道数
	int dst_nb_channels = 0;             // 布局通道数
	int dst_linesize = 0;                // 缓冲区大小 (采样点*通道数*采样位数) 采样位数和音频格式有关
	int dst_nb_samples = 0;              // 采样点
	uint8_t **dst_data = NULL;           // 数据缓冲区

	int frame_size;                      // 一个采样单元占用的字节数（比如2通道时，则左右通道各采样一次合成一个采样单元）
	int bytes_per_sec;                   // 一秒时间的字节数，比如采样率48Khz，2 channel，16bit，则一秒48000*2*16/8=192000

}audio_resampler_params_t;

class AudioResample {

public:
	AudioResample();
	~AudioResample();

	/*
	* @brief 设置重采样参数
	* @return 
	*/
	void set_samples_param(audio_resampler_params_t* resampler_param);

	/**
    * @brief 分配重采样器
    * @param resampler_params 重采样的设置参数
    * @return 成功返回结果；失败返回-1
    */
	int audio_resampler_alloc();

	/**
	* @brief 给输入源分配空间
	* @param 
	* @return 成功返回分配的空间大小；失败返回负数
	*/
	int audio_source_samples_alloc();

	/**
	* @brief 给输出源分配空间
	* @param
	* @return 成功返回分配的空间大小；失败返回负数
	*/
	int audio_destination_samples_alloc();

	/**
     * @brief 释放分配的空间
     * @param 
     * @return 
     */
	void free_alloc();

	/**
	 * @brief 音频重采样
	 * @param in_data  输入音频缓冲区指针
	 * @param out_data 输出音频缓冲区指针
	 * @return 得到的输出音频采样点数
	 */
	int do_audio_resampler(uint8_t **in_data, uint8_t **out_data);

	/**
    * @brief 发送要进行重采样的帧
    * @param resampler
    * @param frame
    * @return 这次重采样后得到的采样点数
    */
	int audio_resampler_send_frame(AVFrame* frame);

	/**
     * @brief 发送要进行重采样的帧
     * @param in_data 二级指针
     * @param in_nb_samples 输入的采样点数量(单个通道)
     * @param pts       pts
     * @return 这次重采样后得到的采样点数
     */
	int audio_resampler_send_frame(uint8_t** in_data, int in_nb_samples, int64_t pts);

	/**
     * @brief 发送要进行重采样的帧
     * @param resampler
     * @param in_data 一级指针
     * @param in_bytes 传入数据的字节大小
     * @param pts
     * @return
     */
	int audio_resampler_send_frame_byte(uint8_t* in_data, int in_bytes, int64_t pts);

	/**
     * @brief 获取重采样后的数据
     * @param nb_samples    我们需要读取多少采样数量: 如果nb_samples>0，只有audio fifo>=nb_samples才能获取到数据;nb_samples=0,有多少就给多少
     *
     * @return 如果获取到采样点数则非NULL
     */
	AVFrame *audio_resampler_receive_frame(int nb_samples);

	/**
     * @brief 获取重采样后的数据
     * @param out_data 缓存获取到的重采样数据，buf一定要充足
     * @param nb_samples 我们需要读取多少采样数量: 如果nb_samples>0，只有audio fifo>=nb_samples才能获取到数据;nb_samples=0,有多少就给多少
     * @param pts  获取帧的pts值
     * @return 获取到的采样点数量
     */
	int audio_resampler_receive_frame(uint8_t **out_data, int nb_samples, int64_t *pts);

	/*
	* @beief flush 处理 补偿最后一音频帧 (只有对数据完整度要求非常高的场景才需要做flush)
	* @param 
	*/
	int audio_resampler_flush(uint8_t **out_data);

	/**
     * @brief audio_resampler_get_fifo_size
     * @return audio fifo的缓存的采样点数量
     */
	int audio_resampler_get_fifo_size();

	 /**
	 * @brief audio_resampler_get_start_pts
	 * @return 起始的pts
	 */
	int64_t audio_resampler_get_start_pts();

	/**
	 * @brief audio_resampler_get_cur_pts
	 * @return 当前的pts
	 */
	int64_t audio_resampler_get_cur_pts();

	/*
	* @brief 打开保存重采样后音频文件
	* @return < 0 打开文件失败
	*/
	int open_dst_file(std::string filename);

	/*
	* @brief 打开文件获取音频文件 编解码器
	* @return < 0 获取编解码器失败
	*/
	int open_audio_info_codec(std::string filename);

	/*
	* @brief 获取原始音频帧
	* @return < 0 获取失败
	*/
	int get_audio_frame(AVFrame* frame);

	/*
	* @beief 获取输出音频文件指针
	* @return 返回输出文件指针
	*/
	FILE* get_out_file() { return m_dst_file; }

private:
	// 获取音频帧
	AVFrame* get_one_frame(const int nb_samples);
	// 创建音频帧内存
	AVFrame *alloc_out_frame(const int nb_samples, const audio_resampler_params_t *resampler_params);
	// 是否需要重采样
	bool is_need_resampler();
	// 初始化采样器内存空间
	int init_resampled_data();
	// 写文件错误
	void write_file_error(int ret_size);

private:
	audio_resampler_params_t* m_resameler_params = NULL;  // 重采样设置参数
	AVFormatContext* m_av_format_context = NULL;          // 重采样上下文
	AVCodecContext*  m_av_codec_context = NULL;           // 重采样解码器上下文
	int m_stream_index = 0;

	struct SwrContext* m_swr_ctx = NULL;          // 重采样核心
	int m_is_fifo_only = 0;                       // 不需要做重采样, 只需要缓存到 audio_fifo
	int m_is_flushed = 0;                         // flush的时候使用

	AVAudioFifo* m_audio_fifo = NULL;             // 重采样点的缓存
	int64_t m_start_pts = AV_NOPTS_VALUE;         // 起始pts
	int64_t m_cur_pts = AV_NOPTS_VALUE;           // 当前pts

	uint8_t** m_resameler_data = NULL;            // 用来缓存重采样后的数据
	int m_resampled_data_size = 0;                // 重采样后的采样数
	int m_src_channels = 0;                       // 输入的通道数
	int m_dst_channels = 0;                       // 输出的通道数
	int64_t m_total_resampled_num = 0;            // 统计总结的采样点数

	int m_audio_frame = 0;                        // 音频帧序号
	int m_in_pts = 0;                             // 输入pts
	int64_t m_out_pts = 0;                        // 输出pts

	// 输出文件
	FILE* m_dst_file = NULL;
	const char* m_dst_filename = NULL;
	const char* m_dst_fmt = NULL;

	const char* m_src_filename = NULL;
};

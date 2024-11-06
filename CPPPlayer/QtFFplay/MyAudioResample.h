#pragma once

/*
* ffmpeg ��Ƶ�ز�����װ
* ���ߣ�dk
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

// �ز����Ĳ���
typedef struct audio_resampler_params {
	// �������
	enum AVSampleFormat src_sample_fmt;  // �����ʽ
	int src_sample_rate = 0;             // ���������
	uint64_t src_channel_layout = 0;     // ����ͨ����
	int src_nb_channels = 0;             // ����ͨ����
	int src_linesize = 0;                // ��������С (������*ͨ����*����λ��) ����λ������Ƶ��ʽ�й�
	int src_nb_samples = 0;              // ������
	uint8_t **src_data = NULL;           // ���ݻ�����

	// �������
	enum AVSampleFormat dst_sample_fmt;  // �����ʽ
	int dst_sample_rate = 0;             // ���������
	uint64_t dst_channel_layout = 0;     // ���ͨ����
	int dst_nb_channels = 0;             // ����ͨ����
	int dst_linesize = 0;                // ��������С (������*ͨ����*����λ��) ����λ������Ƶ��ʽ�й�
	int dst_nb_samples = 0;              // ������
	uint8_t **dst_data = NULL;           // ���ݻ�����

	int frame_size;                      // һ��������Ԫռ�õ��ֽ���������2ͨ��ʱ��������ͨ��������һ�κϳ�һ��������Ԫ��
	int bytes_per_sec;                   // һ��ʱ����ֽ��������������48Khz��2 channel��16bit����һ��48000*2*16/8=192000

}audio_resampler_params_t;

class AudioResample {

public:
	AudioResample();
	~AudioResample();

	/*
	* @brief �����ز�������
	* @return 
	*/
	void set_samples_param(audio_resampler_params_t* resampler_param);

	/**
    * @brief �����ز�����
    * @param resampler_params �ز��������ò���
    * @return �ɹ����ؽ����ʧ�ܷ���-1
    */
	int audio_resampler_alloc();

	/**
	* @brief ������Դ����ռ�
	* @param 
	* @return �ɹ����ط���Ŀռ��С��ʧ�ܷ��ظ���
	*/
	int audio_source_samples_alloc();

	/**
	* @brief �����Դ����ռ�
	* @param
	* @return �ɹ����ط���Ŀռ��С��ʧ�ܷ��ظ���
	*/
	int audio_destination_samples_alloc();

	/**
     * @brief �ͷŷ���Ŀռ�
     * @param 
     * @return 
     */
	void free_alloc();

	/**
	 * @brief ��Ƶ�ز���
	 * @param in_data  ������Ƶ������ָ��
	 * @param out_data �����Ƶ������ָ��
	 * @return �õ��������Ƶ��������
	 */
	int do_audio_resampler(uint8_t **in_data, uint8_t **out_data);

	/**
    * @brief ����Ҫ�����ز�����֡
    * @param resampler
    * @param frame
    * @return ����ز�����õ��Ĳ�������
    */
	int audio_resampler_send_frame(AVFrame* frame);

	/**
     * @brief ����Ҫ�����ز�����֡
     * @param in_data ����ָ��
     * @param in_nb_samples ����Ĳ���������(����ͨ��)
     * @param pts       pts
     * @return ����ز�����õ��Ĳ�������
     */
	int audio_resampler_send_frame(uint8_t** in_data, int in_nb_samples, int64_t pts);

	/**
     * @brief ����Ҫ�����ز�����֡
     * @param resampler
     * @param in_data һ��ָ��
     * @param in_bytes �������ݵ��ֽڴ�С
     * @param pts
     * @return
     */
	int audio_resampler_send_frame_byte(uint8_t* in_data, int in_bytes, int64_t pts);

	/**
     * @brief ��ȡ�ز����������
     * @param nb_samples    ������Ҫ��ȡ���ٲ�������: ���nb_samples>0��ֻ��audio fifo>=nb_samples���ܻ�ȡ������;nb_samples=0,�ж��پ͸�����
     *
     * @return �����ȡ�������������NULL
     */
	AVFrame *audio_resampler_receive_frame(int nb_samples);

	/**
     * @brief ��ȡ�ز����������
     * @param out_data �����ȡ�����ز������ݣ�bufһ��Ҫ����
     * @param nb_samples ������Ҫ��ȡ���ٲ�������: ���nb_samples>0��ֻ��audio fifo>=nb_samples���ܻ�ȡ������;nb_samples=0,�ж��پ͸�����
     * @param pts  ��ȡ֡��ptsֵ
     * @return ��ȡ���Ĳ���������
     */
	int audio_resampler_receive_frame(uint8_t **out_data, int nb_samples, int64_t *pts);

	/*
	* @beief flush ���� �������һ��Ƶ֡ (ֻ�ж�����������Ҫ��ǳ��ߵĳ�������Ҫ��flush)
	* @param 
	*/
	int audio_resampler_flush(uint8_t **out_data);

	/**
     * @brief audio_resampler_get_fifo_size
     * @return audio fifo�Ļ���Ĳ���������
     */
	int audio_resampler_get_fifo_size();

	 /**
	 * @brief audio_resampler_get_start_pts
	 * @return ��ʼ��pts
	 */
	int64_t audio_resampler_get_start_pts();

	/**
	 * @brief audio_resampler_get_cur_pts
	 * @return ��ǰ��pts
	 */
	int64_t audio_resampler_get_cur_pts();

	/*
	* @brief �򿪱����ز�������Ƶ�ļ�
	* @return < 0 ���ļ�ʧ��
	*/
	int open_dst_file(std::string filename);

	/*
	* @brief ���ļ���ȡ��Ƶ�ļ� �������
	* @return < 0 ��ȡ�������ʧ��
	*/
	int open_audio_info_codec(std::string filename);

	/*
	* @brief ��ȡԭʼ��Ƶ֡
	* @return < 0 ��ȡʧ��
	*/
	int get_audio_frame(AVFrame* frame);

	/*
	* @beief ��ȡ�����Ƶ�ļ�ָ��
	* @return ��������ļ�ָ��
	*/
	FILE* get_out_file() { return m_dst_file; }

private:
	// ��ȡ��Ƶ֡
	AVFrame* get_one_frame(const int nb_samples);
	// ������Ƶ֡�ڴ�
	AVFrame *alloc_out_frame(const int nb_samples, const audio_resampler_params_t *resampler_params);
	// �Ƿ���Ҫ�ز���
	bool is_need_resampler();
	// ��ʼ���������ڴ�ռ�
	int init_resampled_data();
	// д�ļ�����
	void write_file_error(int ret_size);

private:
	audio_resampler_params_t* m_resameler_params = NULL;  // �ز������ò���
	AVFormatContext* m_av_format_context = NULL;          // �ز���������
	AVCodecContext*  m_av_codec_context = NULL;           // �ز���������������
	int m_stream_index = 0;

	struct SwrContext* m_swr_ctx = NULL;          // �ز�������
	int m_is_fifo_only = 0;                       // ����Ҫ���ز���, ֻ��Ҫ���浽 audio_fifo
	int m_is_flushed = 0;                         // flush��ʱ��ʹ��

	AVAudioFifo* m_audio_fifo = NULL;             // �ز�����Ļ���
	int64_t m_start_pts = AV_NOPTS_VALUE;         // ��ʼpts
	int64_t m_cur_pts = AV_NOPTS_VALUE;           // ��ǰpts

	uint8_t** m_resameler_data = NULL;            // ���������ز����������
	int m_resampled_data_size = 0;                // �ز�����Ĳ�����
	int m_src_channels = 0;                       // �����ͨ����
	int m_dst_channels = 0;                       // �����ͨ����
	int64_t m_total_resampled_num = 0;            // ͳ���ܽ�Ĳ�������

	int m_audio_frame = 0;                        // ��Ƶ֡���
	int m_in_pts = 0;                             // ����pts
	int64_t m_out_pts = 0;                        // ���pts

	// ����ļ�
	FILE* m_dst_file = NULL;
	const char* m_dst_filename = NULL;
	const char* m_dst_fmt = NULL;

	const char* m_src_filename = NULL;
};

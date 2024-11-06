#pragma once

/*
* QtFFplay
* ��Ҫ���������⸴�ã��������з�������Ƶpacket�������뻺�����
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
	MEDIA_IDLE = 0, // Ĭ��״̬
	MEDIA_START,    // ��ʼ����
	MEDIA_STOP,     // ֹͣ����
	MEDIA_PAUSE     // ��ͣ����
}MEDIA_COMMAND;

class AVPlayer {
public:
	AVPlayer();
	~AVPlayer();

	/*
	* @brief ��ý���ļ� �ص�״̬��
	*/
	RET_CODE play(std::string filename, StartplayCallBack cb);

	/*
	* @brief �ر�ý���ļ� �ص�״̬��
	*/
	void stop();

	/*
	*  @brief ��ͣ
	*/
	void Pause();

	//libsonic ���ٱ������ �� libsoundtouch 
	/**
	* @brief seek in the stream
    * @param pos  ����seek����λ��
    * @param rel  �������
    * @param seek_by_bytes
	*/
	void Seek(int64_t pos, int64_t rel, int seek_by_bytes);

	/*
	* @brief ���ٲ��� ���ٲ������ʹ��sonic��
	*/
	void Rate(double speed);

	/*
	* @brief ��ȡ�ļ�����ʱ��
	*/
	int64_t get_file_duration() { if (m_avformat_context == nullptr) return 0; else return m_avformat_context->duration; }

	/*
	* @brief ��ȡ��һ֡��λ��
	*/
	int64_t get_file_starttime() { return m_avformat_context->start_time; }

	/*
	* @brief ��ȡ�����ļ����ֽ�
	*/
	uint64_t get_avio_size() { return avio_size(m_avformat_context->pb); }

	/*
	* @brief ��ȡ�ļ��Ѳ���ʱ��
	*/
	double get_master_clock() { return m_av_clock.get_master_clock(); }

	/*
    * @beief ��ȡ��Ƶ֡��
    */
	int getFps() { return m_fps; }

	/*
	* @beief �Ƿ�����Ƶ�� true: �� false: û��
	*/
	bool IsHaveVideoStream() { return (m_video_stream != -1); }

	/*
	* @beief �Ƿ�����Ƶ�� true: �� false: û��
	*/
	bool IsHaveAudioStream() { return (m_audio_stream != -1); }

	/*
	* @beief ��ȡ��Ƶ������
	*/
	FFDecoder* getVideoDecoder() { return &m_video_decoder; }

	/*
	* @beief ��ȡ��Ƶ������
	*/
	FFDecoder* getAudioDecoder() { return &m_audio_decoder; }

	cvpublish::AVDecoder* getVidioDecode();
	cvpublish::AVDecoder* getAudioDecode();

	/*
	*  ���ò������¼��ص�ָ��
	*/
	void setMediaPlayerEventHandler(MediaPlayerEventHandler* eventHandler) { m_mediaplayerEventHandler = eventHandler; }

private:
	// ������ָ��
	void mediaCommand();
	// ��ʼ����
	RET_CODE startPlay();
	// ��������
	void stopPlay();
	// ��ȡ�����߳�
	void read_thread();
	// ���������
	RET_CODE allocation_decoder(FFDecoder* coder, int stream);
	// �ͷŽ�����
	RET_CODE release_decoder(FFDecoder* coder);
	// �������
	static int interrupt_cb(void *ctx);
	// ����״̬��
	void setPlayerStateChanged(MediaPlayerState state, MediaPlayerError error);
	//  
	void Release();
private:
	std::string m_file_name;
	StartplayCallBack m_startplay_callback = nullptr;

	//video param ��Ҫ����װ
	int m_video_stream = -1;
	int m_fps = 0;
	int m_video_index = 0;

	// audio param ��Ҫ����װ
	int m_audio_stream = -1;

	// av_read_frame ��ȡ��avpacket
	AVPacket* m_avpacket = nullptr;

	std::thread* m_async_thread = nullptr; // �첽�����߳�
	std::thread* m_read_thread = nullptr;  // ��ȡ�����߳�
	std::mutex* m_wait_mutex = nullptr;    // ��ȡ������
	std::condition_variable* m_cond_t_read_thread = nullptr; // ���Ѷ�ȡ�����߳���������

	// ��Ƶ����
	bool m_isplay = false;
	bool m_paused = false; // ��ͣ
	bool m_playInit = false;
	int  m_eof = 0;        // �Ƿ��ȡ����

	// seek
	int	    m_seek_req = 0;    // ��ʶһ��seek����
	int	    m_seek_flags = AVSEEK_FLAG_BYTE;  // seek��־������AVSEEK_FLAG_BYTE��
	int64_t	m_seek_pos = 0;    // ����seek��Ŀ��λ��(��ǰλ��+����)
	int64_t	m_seek_rel = 0;    // ����seek��λ������

	AVFormatContext* m_avformat_context = nullptr;   // iformat��������

	FFDecoder m_video_decoder; // ��Ƶ������
	FFDecoder m_audio_decoder; // ��Ƶ������

	AVPacketQueue m_avpacket_queue; // ����Ƶ����

	cvpublish::AVDecoder* m_video_decode_thread = NULL; // ��Ƶ�����߳�
	cvpublish::AVDecoder* m_audio_decode_thread = NULL; // ��Ƶ�����߳�

	AVClock m_av_clock; // ʱ��

	bool m_bReadFrame = true; // ��϶���������av_read_frame����
	int m_timeoutReadFrame = 0; // ��϶���������av_read_frame����
	uint64_t m_readframe_callback_time = 0;

	MediaPlayerEventHandler* m_mediaplayerEventHandler = nullptr;
	MEDIA_COMMAND m_media_command = MEDIA_IDLE;
	std::mutex* m_command_mutex_t = nullptr;
	std::condition_variable* m_command_cond_t = nullptr;
	bool m_blruncommand = false;
};
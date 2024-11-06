#include "AVPlayer.h"
#include <QDebug>

AVPlayer::AVPlayer() {
	m_command_mutex_t = new std::mutex;
	m_command_cond_t = new std::condition_variable;
	m_blruncommand = true;
	m_async_thread = new std::thread(&AVPlayer::mediaCommand, this);
}

AVPlayer::~AVPlayer() {
	qDebug() << "AVPlayer::~AVPlayer()";
	m_bReadFrame = false;
	m_blruncommand = false;
	m_command_cond_t->notify_one();
	m_async_thread->join();
	delete m_async_thread;
	delete m_command_mutex_t;
	delete m_command_cond_t;
}

RET_CODE AVPlayer::play(std::string filename, StartplayCallBack cb) {
	//if (m_media_command != MEDIA_IDLE)
	//	return RET_FAIL;

	m_startplay_callback = cb;
	m_file_name = filename;
	m_media_command = MEDIA_START;
	m_command_cond_t->notify_one();
	return RET_OK;
}

void AVPlayer::stop() {
	//if (m_media_command != MEDIA_IDLE)
	//	return;

	m_media_command = MEDIA_STOP;
	m_command_cond_t->notify_one();
	//stopPlay();
}

void AVPlayer::mediaCommand() {
	while (m_blruncommand)
	{
		std::unique_lock<std::mutex> lock(*m_command_mutex_t);
		m_command_cond_t->wait(lock);
		int command = m_media_command;

		for(;;)
		{
			switch (command)
			{
			case MEDIA_START:
				startPlay();
				break;
			case MEDIA_STOP:
				stopPlay();
				break;
			}
			if (command != m_media_command) {
				command = m_media_command;
				continue;
			}
			break;
		}
	}
}

RET_CODE AVPlayer::startPlay() {
	if (m_isplay)
		return RET_FAIL;

	LogInfo("start play file: %s", m_file_name.c_str());

	int ret;
	int num_butes;
	m_bReadFrame = true;

	//m_file_name = filename;
	m_avformat_context = avformat_alloc_context();

	// ע��������ϵĻص�����
	m_avformat_context->interrupt_callback.callback = interrupt_cb;
	m_avformat_context->interrupt_callback.opaque = this;

	// ffmpegȡrtsp��ʱav_read_frame�����Ľ���취 ���ò����Ż�
	AVDictionary* avdic = NULL;
	av_dict_set(&avdic, "buffer_size", "102400", 0); //���û����С��1080p�ɽ�ֵ����
	av_dict_set(&avdic, "rtmp_transport", "udp", 0); //��udp��ʽ�򿪣������tcp��ʽ�򿪽�udp�滻Ϊtcp
	av_dict_set(&avdic, "stimeout", "2000000", 0);   //���ó�ʱ�Ͽ�����ʱ�䣬��λ΢��
	av_dict_set(&avdic, "max_delay", "500000", 0);   //�������ʱ��

	ONPLAYERSTATECHANGED_EVENT(PLAYER_STATE_OPENING, PLAYER_ERROR_NONE);

	// ���ļ�
	if ((ret = avformat_open_input(&m_avformat_context, m_file_name.c_str(), NULL, &avdic)) < 0) {
		LogError("avformat_open_input failed:%d\n", ret);
		ONPLAYERSTATECHANGED_EVENT(PLAYER_STATE_FAILED, PLAYER_ERROR_NO_RESOURCE);
		return RET_ERR_OPEN_FILE;
	}

	if (avformat_find_stream_info(m_avformat_context, NULL) < 0) {
		LogError("avformat_find_stream_info failed:%d\n", ret);
		setPlayerStateChanged(PLAYER_STATE_FAILED, PLAYER_ERROR_INVALID_MEDIA_SOURCE);
		return RET_FAIL;
	}

	ONPLAYERSTATECHANGED_EVENT(PLAYER_STATE_OPEN_COMPLETED, PLAYER_ERROR_NONE);

	// ������Ƶ��
	for (unsigned i = 0; i < m_avformat_context->nb_streams; i++) {
		if (m_avformat_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			m_video_stream = i;
			break;
		}
	}

	// ������Ƶ��
	for (unsigned i = 0; i < m_avformat_context->nb_streams; ++i) {
		enum AVMediaType av_media_type = m_avformat_context->streams[i]->codecpar->codec_type;
		if (av_media_type == AVMEDIA_TYPE_AUDIO) {
			m_audio_stream = i;
			break;
		}
	}

	// ���г�ʼ��
	m_avpacket_queue.frame_queue_init(m_avpacket_queue.get_frame_video_queue(), m_avpacket_queue.get_video_packet_point(), VIDEO_PICTURE_QUEUE_SIZE, 1);
	m_avpacket_queue.frame_queue_init(m_avpacket_queue.get_frame_audio_queue(), m_avpacket_queue.get_audio_packet_point(), SAMPLE_QUEUE_SIZE, 1);
	m_avpacket_queue.packet_vidio_queue_init();
	m_avpacket_queue.packet_audio_queue_init();

	// ��ȡ��Ƶ����
	if (m_video_stream != -1) { // streams[m_video_stream]->r_frame_rate
		// ��ȡ֡��
		m_fps = m_avformat_context->streams[m_video_stream]->r_frame_rate.num /
			m_avformat_context->streams[m_video_stream]->r_frame_rate.den;

		m_avpacket_queue.packet_video_queue_start();
		allocation_decoder(&m_video_decoder, m_video_stream);
		// ������Ƶ�����߳�
		m_video_decode_thread = new cvpublish::AVDecoder(&m_video_decoder, &m_avpacket_queue, &m_av_clock);
		m_video_decode_thread->Start();
	}

	// ��ȡ��Ƶ����
	if (m_audio_stream != -1) {
		m_avpacket_queue.packet_audio_queue_start();
		allocation_decoder(&m_audio_decoder, m_audio_stream);
		// ������Ƶ�����߳�
		m_audio_decode_thread = new cvpublish::AVDecoder(&m_audio_decoder, &m_avpacket_queue, &m_av_clock);
		m_audio_decode_thread->Start();
	}

	//�����Ƶ��Ϣ
	av_dump_format(m_avformat_context, 0, m_file_name.c_str(), 0);

	/*
	* ��ʼ��ʱ��
	* ʱ������->queue_serial��ʵ����ָ�����is->videoq.serial
	*/
	m_av_clock.init_clock(&m_av_clock.vidclk, &m_avpacket_queue.get_video_packet_point()->serial);
	m_av_clock.init_clock(&m_av_clock.audclk, &m_avpacket_queue.get_audio_packet_point()->serial);
	m_av_clock.init_clock(&m_av_clock.extclk, &m_av_clock.extclk.serial);

	m_av_clock.max_frame_duration = (m_avformat_context->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

	m_wait_mutex = new std::mutex;
	m_cond_t_read_thread = new std::condition_variable;
	// new packet
	m_avpacket = (AVPacket *)av_malloc(sizeof(AVPacket));
	av_init_packet(m_avpacket);

	if (m_startplay_callback != nullptr) {
		m_startplay_callback();
	}

	// ��ʼ����
	m_readframe_callback_time = 0;
	m_isplay = true;
	m_read_thread = new std::thread(&AVPlayer::read_thread, this);
	LogInfo("play success.");
	ONPLAYERSTATECHANGED_EVENT(PLAYER_STATE_PLAYING, PLAYER_ERROR_NONE);
	return RET_OK;
}

void AVPlayer::stopPlay() {
	//if (!m_isplay) {
	//	qDebug() << "AVPlayer::stopPlay() m_isplay is false;";
	//	return;
	//}

	qDebug() << "AVPlayer::stopPlay() start;";
	m_bReadFrame = false; // �������
	m_isplay = false; // ������־
	// �������ݶ�ȡ�߳�
	if (m_read_thread != nullptr) {
		m_read_thread->join();
		delete m_cond_t_read_thread;
		delete m_read_thread;
		m_read_thread = nullptr;
		m_cond_t_read_thread = nullptr;
	}
	Release();
	ONPLAYERSTATECHANGED_EVENT(PLAYER_STATE_PLAYBACK_END_COMPLETED, PLAYER_ERROR_NONE);
}

void AVPlayer::Pause() {
	if (!m_isplay)
		return;

	m_paused = !m_paused;
	m_video_decode_thread->Pause();
	m_audio_decode_thread->Pause();
}

void AVPlayer::Seek(int64_t pos, int64_t rel, int seek_by_bytes) {
	if (!m_isplay)
		return;

	if (!m_seek_req) {
		m_seek_pos = pos;
		m_seek_rel = rel;
		m_seek_flags &= ~AVSEEK_FLAG_BYTE; // �����ֽڵķ�ʽȥseek
		if (seek_by_bytes)
			m_seek_flags |= AVSEEK_FLAG_BYTE; // ǿ�ư��ֽڵķ�ʽȥseek
		m_seek_req = 1;  // ����seek�� ��read_thread�߳�seek�ɹ��Ž�����Ϊ0
		m_cond_t_read_thread->notify_all(); // ����
	}
}

void AVPlayer::Rate(double speed) {
	if (!m_isplay)
		return;

	m_video_decode_thread->setPlaybackRate(speed);
	m_audio_decode_thread->setPlaybackRate(speed);
}

RET_CODE AVPlayer::allocation_decoder(FFDecoder* coder, int stream) {
	int ret;

	AVCodecParameters* pavcodec_parameters = m_avformat_context->streams[stream]->codecpar;

	// ���ҽ����� ����Ƶ��(����Ļ��)��Ҫʹ�ò�ͬ�Ľ�����
	coder->codec_context = avcodec_alloc_context3(NULL);
	if (!coder->codec_context) {
		return RET_ERR_MISMATCH_CODE;
	}
	ret = avcodec_parameters_to_context(coder->codec_context, pavcodec_parameters);
	if (ret < 0) {
		return RET_FAIL;
	}
	coder->avcodec = avcodec_find_decoder(coder->codec_context->codec_id);
	if (!coder->avcodec) {
		return RET_ERR_MISMATCH_CODE;
	}

	// �򿪽�����
	if (avcodec_open2(coder->codec_context, coder->avcodec, NULL) < 0) {
		return RET_FAIL;
	}

	// init_decoder
	coder->avformat_context = m_avformat_context;
	if (coder->codec_context->codec_type == AVMEDIA_TYPE_AUDIO) {
		coder->start_pts = m_avformat_context->streams[m_audio_stream]->start_time;
		coder->start_pts_tb = m_avformat_context->streams[m_audio_stream]->time_base;
		coder->codec_context->pkt_timebase = m_avformat_context->streams[m_audio_stream]->time_base;
		coder->queue = m_avpacket_queue.get_audio_packet_point();
	}
	else if (coder->codec_context->codec_type == AVMEDIA_TYPE_VIDEO) {
		coder->queue = m_avpacket_queue.get_video_packet_point();
		coder->video_st = m_avformat_context->streams[m_video_index];
	}

	return RET_OK;
}

RET_CODE AVPlayer::release_decoder(FFDecoder* coder) {
	if (coder->codec_context != nullptr && coder->codec_context->codec_type == AVMEDIA_TYPE_AUDIO) {
		m_avpacket_queue.packet_audio_queue_about();
		m_avpacket_queue.frame_audio_queue_signal();
		m_audio_decode_thread->Stop(); // ������Ƶ�����߳�
		delete m_audio_decode_thread;
		m_audio_decode_thread = NULL;
		m_avpacket_queue.packet_audio_queue_flash();
	}
	else if (coder->codec_context != nullptr && coder->codec_context->codec_type == AVMEDIA_TYPE_VIDEO) {
		m_avpacket_queue.packet_video_queue_about();
		m_avpacket_queue.frame_video_queue_signal();
		m_video_decode_thread->Stop(); // ������Ƶ�����߳�
		delete m_video_decode_thread;
		m_video_decode_thread = NULL;
		coder->video_st = nullptr;
		m_avpacket_queue.packet_video_queue_flash();
	}
	if (coder->codec_context != nullptr) {
		//av_packet_unref(&coder->pkt);
		avcodec_free_context(&coder->codec_context);
	}
	return RET_OK;
}

int AVPlayer::interrupt_cb(void *ctx)
{
	AVPlayer* avplayer = (AVPlayer*)ctx;
	if (avplayer == nullptr)
		return 0;

	if (!avplayer->m_bReadFrame)
		return 1; // ��϶���������av_read_frame����

	if (avplayer->m_bReadFrame && avplayer->m_readframe_callback_time != 0)
	{
		auto now_callback_time = av_gettime_relative();
		int64_t difftime = now_callback_time - avplayer->m_readframe_callback_time;
		if (difftime < 1 * 5000000) { // 5�볬ʱ
			//++(pFFPlayer->m_timeoutReadFrame);
			//qDebug() << "difftime: " << difftime;
		}
		else {
			return 1; // ��϶���������av_read_frame����
		}
	}

	return 0;
}

void AVPlayer::setPlayerStateChanged(MediaPlayerState state, MediaPlayerError error) {
	if (m_mediaplayerEventHandler != nullptr) {
		m_mediaplayerEventHandler->onPlayerStateChange(state, error);
		LogInfo("PlayerState state:%d, error:%d", state, error);
	}
}

void AVPlayer::Release() {

	// �ͷŽ�����
	release_decoder(&m_audio_decoder);
	release_decoder(&m_video_decoder);

	// ����pakcet����
	m_avpacket_queue.packet_video_queue_destroy();
	m_avpacket_queue.packet_audio_queue_destroy();

	// ����frame����
	m_avpacket_queue.frame_queue_video_destory();
	m_avpacket_queue.frame_queue_audio_destory();

	if (m_avformat_context != nullptr) {
		avformat_close_input(&m_avformat_context);
		avformat_free_context(m_avformat_context);
		m_avformat_context = nullptr;
	}
	if (m_avpacket != nullptr) {
		av_free(m_avpacket);
		m_avpacket = nullptr;
	}

	m_readframe_callback_time = 0;
	m_fps = 0;
	m_video_stream = -1;
	m_audio_stream = -1;
	m_paused = false;
}

cvpublish::AVDecoder* AVPlayer::getVidioDecode() 
{ 
	if (m_isplay) {
		return m_video_decode_thread;
	}
	return nullptr;
}

cvpublish::AVDecoder* AVPlayer::getAudioDecode() 
{
	if (m_isplay) {
		return m_audio_decode_thread;
	}
	return nullptr;
}

void AVPlayer::read_thread() {
	while(m_isplay){
		int ret;
		if (m_seek_req) { // �Ƿ���seek����
			int64_t seek_target = m_seek_pos;
			int64_t seek_min = m_seek_rel > 0 ? seek_target - m_seek_rel + 2 : INT64_MIN;
			int64_t seek_max = m_seek_rel < 0 ? seek_target = m_seek_rel - 2 : INT64_MAX;

			ret = avformat_seek_file(m_avformat_context, -1, seek_min, seek_target, seek_max, m_seek_flags);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "%s: error while seeking\n", m_avformat_context->url);
			}
			else {
				/* seek��ʱ��Ҫ��ԭ�ȵ�������գ���������������
				 * put flush_pkt��Ŀ���Ǹ�֪�����߳���Ҫreset decoder
				 */
				if (m_audio_stream >= 0) {  // �������Ƶ��
					m_avpacket_queue.packet_audio_queue_flash();
					m_avpacket_queue.audio_queue_put_flash();
				}
				if (m_video_stream >= 0) { // �������Ƶ��
					m_avpacket_queue.packet_video_queue_flash();
					m_avpacket_queue.video_queue_put_flash();
				}
				m_seek_req = 0;
				//queue_attachments_req = 1; // ����
				m_eof = 0; 
			}
		}

		// �ж϶������ֵ,�˴��ж϶����Ƿ����㹻�����ݣ���������
		if (m_paused || m_avpacket_queue.get_video_packet_size() + m_avpacket_queue.get_audio_packet_size() > MAX_QUEUE_SIZE) {
			// wait 10 ms
			// seek ����ʱ�ᱻ����
			std::unique_lock<std::mutex> lock(*m_wait_mutex);
			m_cond_t_read_thread->wait_for(lock, std::chrono::milliseconds(10));
			continue;		// ����ѭ��
		}

		// ��������Ƿ��Ѿ����Ž���
		//if(!m_paused && )

		//m_bReadFrame = true; // ��϶���������av_read_frame����
		m_timeoutReadFrame = 0; // ��϶���������av_read_frame����
		m_readframe_callback_time = av_gettime_relative();

		// ��ȡý������
		ret = av_read_frame(m_avformat_context, m_avpacket); // ÿһ�� av_read_frame ����Ҫ av_packet_unref �ͷ��ڴ�

		// ��������Ƿ��ȡ���
		if (ret < 0) {
			//m_bReadFrame = false;
			if ((ret == AVERROR_EOF || avio_feof(m_avformat_context->pb)) && !m_eof) {
				// ����հ�˵���������ݶ�ȡ��ϣ�Ϊ�˴ӽ�������˵��֡��������
				if (m_video_stream >= 0) {
					m_avpacket_queue.video_queue_put_nullpacket(m_video_stream);
				}
				if (m_audio_stream >= 0) {
					m_avpacket_queue.audio_queue_put_nullpacket(m_audio_stream);
				}
				m_eof = 1; // �ļ���ȡ���
			}
			if (m_avformat_context->pb && m_avformat_context->pb->error) {
				LogInfo("av_read_frame pb & error. errorcode: %d", m_avformat_context->pb->error);
				if (m_bReadFrame) { // ��ʱ
					ONPLAYERSTATECHANGED_EVENT(PLAYER_STATE_FAILED, PLAYER_ERROR_READFRAME_TIMEOUT);
				}
				else { // ��������� ǿ�ƽ�������
					ONPLAYERSTATECHANGED_EVENT(PLAYER_STATE_PLAYBACK_COMPLETED, PLAYER_ERROR_NONE);
				}
				//m_isplay = false;
				break; // thread end
			}

			std::unique_lock<std::mutex> lock(*m_wait_mutex);
			m_cond_t_read_thread->wait_for(lock, std::chrono::milliseconds(10));
			continue; // ����ѭ��
		}
		//m_bReadFrame = false;

		if (m_avpacket->stream_index == m_video_stream) { // ��Ƶ��
			m_avpacket_queue.packet_video_queue_put(m_avpacket);
		}
		else if (m_avpacket->stream_index == m_audio_stream) { // ��Ƶ��
			m_avpacket_queue.packet_audio_queue_put(m_avpacket);
		}
		else {
			av_packet_unref(m_avpacket);
		}
	}
	LogInfo("av_read_frame thread end.");
}
#include "AnalyzeFrame.h"
#include <QDebug>

AnalyzeFrameEngine::AnalyzeFrameEngine()
{
	init_logger("Analyze_Frame.log", S_INFO);
}

AnalyzeFrameEngine::~AnalyzeFrameEngine()
{
	qDebug() << "AnalyzeFrameEngine::~AnalyzeFrameEngine";
}

int AnalyzeFrameEngine::startAnalyze(std::string file_name)
{
	int ret;
	m_file_name = file_name;

	ret = getPicture();

	m_avformat_context = avformat_alloc_context();

	// 打开文件
	if ((ret = avformat_open_input(&m_avformat_context, file_name.c_str(), NULL, NULL)) < 0) {
		LogError("avformat_open_input failed:%d\n", ret);
		return RET_ERR_OPEN_FILE;
	}

	if (avformat_find_stream_info(m_avformat_context, NULL) < 0) {
		LogError("avformat_find_stream_info failed:%d\n", ret);
		return RET_FAIL;
	}


	// 查找视频流
	for (unsigned i = 0; i < m_avformat_context->nb_streams; i++) {
		if (m_avformat_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			m_video_stream = i;
			break;
		}
	}

	// 查找音频流
	for (unsigned i = 0; i < m_avformat_context->nb_streams; ++i) {
		enum AVMediaType av_media_type = m_avformat_context->streams[i]->codecpar->codec_type;
		if (av_media_type == AVMEDIA_TYPE_AUDIO) {
			m_audio_stream = i;
			break;
		}
	}

	// 获取视频参数
	if (m_video_stream != -1) {
		m_fps = m_avformat_context->streams[m_video_stream]->r_frame_rate.num / m_avformat_context->streams[m_video_stream]->r_frame_rate.den;
	}
	return RET_OK;
}

void AnalyzeFrameEngine::clearFrame()
{

}

RET_CODE AnalyzeFrameEngine::allocation_decoder(FFDecoder* coder, int stream) {
	int ret;

	AVCodecParameters* pavcodec_parameters = m_avformat_context->streams[stream]->codecpar;

	// 查找解码器 音视频流(与字幕流)需要使用不同的解码器
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

	// 打开解码器
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

RET_CODE AnalyzeFrameEngine::release_decoder(FFDecoder* coder) {
	if (coder->codec_context != nullptr && coder->codec_context->codec_type == AVMEDIA_TYPE_AUDIO) {
		m_avpacket_queue.packet_audio_queue_about();
		m_avpacket_queue.frame_audio_queue_signal();
		m_audio_decode_thread->Stop(); // 结束音频解码线程
		delete m_audio_decode_thread;
		m_audio_decode_thread = NULL;
		m_avpacket_queue.packet_audio_queue_flash();
	}
	else if (coder->codec_context != nullptr && coder->codec_context->codec_type == AVMEDIA_TYPE_VIDEO) {
		m_avpacket_queue.packet_video_queue_about();
		m_avpacket_queue.frame_video_queue_signal();
		m_video_decode_thread->Stop(); // 结束视频解码线程
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

void AnalyzeFrameEngine::setPlayerStateChanged(MediaPlayerState state, MediaPlayerError error) {
	if (m_mediaplayerEventHandler != nullptr) {
		m_mediaplayerEventHandler->onPlayerStateChange(state, error);
		LogInfo("PlayerState state:%d, error:%d", state, error);
	}
}

void AnalyzeFrameEngine::startDecode(int width, int height)
{
	if (m_isDone)
		return;

	if (width == 0 && height == 0)
	{
		width  = 1280;
		height = 720;
	}

	// 队列初始化
	m_avpacket_queue.frame_queue_init(m_avpacket_queue.get_frame_video_queue(), m_avpacket_queue.get_video_packet_point(), VIDEO_PICTURE_QUEUE_SIZE, 1);
	m_avpacket_queue.frame_queue_init(m_avpacket_queue.get_frame_audio_queue(), m_avpacket_queue.get_audio_packet_point(), SAMPLE_QUEUE_SIZE, 1);
	m_avpacket_queue.packet_vidio_queue_init();
	m_avpacket_queue.packet_audio_queue_init();

	// 获取视频参数
	if (m_video_stream != -1) { // streams[m_video_stream]->r_frame_rate
		// 获取帧率
		m_fps = m_avformat_context->streams[m_video_stream]->r_frame_rate.num /
			m_avformat_context->streams[m_video_stream]->r_frame_rate.den;

		m_avpacket_queue.packet_video_queue_start();
		allocation_decoder(&m_video_decoder, m_video_stream);
		// 创建视频解码线程
		m_video_decoder.out_width = width;
		m_video_decoder.out_height = height;
		m_video_decode_thread = new cvpublish::AVDecoder(&m_video_decoder, &m_avpacket_queue, &m_av_clock);
		m_video_decode_thread->setEndTime(m_end_time);
		m_video_decode_thread->Start();
	}

	// 获取音频参数
	if (m_audio_stream != -1) {
		m_avpacket_queue.packet_audio_queue_start();
		allocation_decoder(&m_audio_decoder, m_audio_stream);
		// 创建音频解码线程
		m_audio_decode_thread = new cvpublish::AVDecoder(&m_audio_decoder, &m_avpacket_queue, &m_av_clock);
		m_audio_decode_thread->setEndTime(m_end_time);
		m_audio_decode_thread->Start();
	}

	/*
* 初始化时钟
* 时钟序列->queue_serial，实际上指向的是is->videoq.serial
*/
	m_av_clock.init_clock(&m_av_clock.vidclk, &m_avpacket_queue.get_video_packet_point()->serial);
	m_av_clock.init_clock(&m_av_clock.audclk, &m_avpacket_queue.get_audio_packet_point()->serial);
	m_av_clock.init_clock(&m_av_clock.extclk, &m_av_clock.extclk.serial);

	m_av_clock.max_frame_duration = (m_avformat_context->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

	m_wait_mutex = new std::mutex;
	m_cond_t_read_thread = new std::condition_variable;

	// new packet
	m_avpacket = (AVPacket*)av_malloc(sizeof(AVPacket));
	av_init_packet(m_avpacket);

	if (m_startplay_callback != nullptr) {
		m_startplay_callback();
	}

	// 开始解码
	m_isDone = true;
	m_read_thread = new std::thread(&AnalyzeFrameEngine::read_thread, this);
}

void AnalyzeFrameEngine::play(StartplayCallBack cb)
{
	m_startplay_callback = cb;

	startDecode();

	if (m_paused) Pause();
}

void AnalyzeFrameEngine::Pause() {
	if (!m_isDone)
		return;

	m_paused = !m_paused;
	m_video_decode_thread->Pause();
	m_audio_decode_thread->Pause();
}

void AnalyzeFrameEngine::Seek(int64_t pos, int64_t rel, int seek_by_bytes) {
	//if (!m_isDone)
	//	return;

	if (!m_seek_req) {

		if (!m_paused) 
			Pause();

		m_seek_pos = pos;
		m_seek_rel = rel;
		m_seek_flags &= ~AVSEEK_FLAG_BYTE; // 不按字节的方式去seek
		if (seek_by_bytes)
			m_seek_flags |= AVSEEK_FLAG_BYTE; // 强制按字节的方式去seek
		m_seek_req = 1;  // 请求seek， 在read_thread线程seek成功才将其置为0
		m_cond_t_read_thread->notify_all(); // 唤醒
	}

	//int64_t seek_target = m_seek_pos;
	//int64_t seek_min = m_seek_rel > 0 ? seek_target - m_seek_rel + 2 : INT64_MIN;
	//int64_t seek_max = m_seek_rel < 0 ? seek_target = m_seek_rel - 2 : INT64_MAX;

	//av_log(NULL, AV_LOG_INFO, "seek_target: %lld\n", seek_target);

	//int ret = avformat_seek_file(m_avformat_context, -1, seek_min, seek_target, seek_max, m_seek_flags);
	//if (ret < 0) {
	//	av_log(NULL, AV_LOG_ERROR, "%s: error while seeking\n", m_avformat_context->url);
	//}
}

std::string AnalyzeFrameEngine::get_file_name()
{ 
	std::string input = m_avformat_context->filename;

	// 找到最后一个 '\' 的位置
	size_t pos = input.find_last_of('\\');
	if (pos == std::string::npos) {
		// 如果没有找到 '\', 返回整个字符串
		return input;
	}

	// 返回 '\' 之后的子字符串
	return input.substr(pos + 1);
}

AVFrame* AnalyzeFrameEngine::getVideoDecodeFrame()
{
	AVFrame* frame = m_video_decode_thread->getVideoAVFrame();
	if (frame == NULL) return NULL;
	return frame;
}

cvpublish::AVDecoder* AnalyzeFrameEngine::getVidioDecode()
{
	if (m_isDone) {
		return m_video_decode_thread;
	}
	return nullptr;
}

cvpublish::AVDecoder* AnalyzeFrameEngine::getAudioDecode()
{
	if (m_isDone) {
		return m_audio_decode_thread;
	}
	return nullptr;
}

AVFrame* AnalyzeFrameEngine::getAudioDecodeFrame()
{
	AVFrame* frame = m_audio_decode_thread->getAudioAVFrame();
	if (frame == NULL) return NULL;

	//double audio_frame_duration = (double)frame->nb_samples / frame->sample_rate;
	//m_audio_total_time += audio_frame_duration * 1000;
	//printf("Audio frame total duration: %lld seconds\n", m_audio_total_time);
	//if (m_audio_total_time >= m_end_time) {
	//	return NULL;
	//}
	return frame;
}

RET_CODE AnalyzeFrameEngine::getPicture()
{
	const char* inputFile = m_file_name.c_str();
	AVFormatContext* formatContext = nullptr;

	// 打开输入文件
	if (avformat_open_input(&formatContext, inputFile, nullptr, nullptr) < 0) {
		std::cerr << "Failed to open input file: " << inputFile << std::endl;
		return RET_FAIL;
	}

	// 查找流信息
	if (avformat_find_stream_info(formatContext, nullptr) < 0) {
		std::cerr << "Failed to find stream info" << std::endl;
		avformat_close_input(&formatContext);
		return RET_FAIL;
	}

	// 查找视频流索引
	int videoStreamIndex = -1;
	for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {
		if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStreamIndex = i;
			break;
		}
	}

	if (videoStreamIndex == -1) {
		std::cerr << "No video stream found" << std::endl;
		avformat_close_input(&formatContext);
		return RET_FAIL;
	}

	AVCodecParameters* codecParams = formatContext->streams[videoStreamIndex]->codecpar;
	const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
	if (!codec) {
		std::cerr << "Failed to find codec" << std::endl;
		avformat_close_input(&formatContext);
		return RET_FAIL;
	}

	AVCodecContext* codecContext = avcodec_alloc_context3(codec);
	if (!codecContext) {
		std::cerr << "Failed to allocate codec context" << std::endl;
		avformat_close_input(&formatContext);
		return RET_FAIL;
	}

	if (avcodec_parameters_to_context(codecContext, codecParams) < 0) {
		std::cerr << "Failed to initialize codec context" << std::endl;
		avcodec_free_context(&codecContext);
		avformat_close_input(&formatContext);
		return RET_FAIL;
	}

	if (avcodec_open2(codecContext, codec, nullptr) < 0) {
		std::cerr << "Failed to open codec" << std::endl;
		avcodec_free_context(&codecContext);
		avformat_close_input(&formatContext);
		return RET_FAIL;
	}

	AVPacket* packet = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();
	AVFrame* rgbFrame = av_frame_alloc();

	rgbFrame->format = AV_PIX_FMT_RGB32;
	rgbFrame->width = codecContext->width;
	rgbFrame->height = codecContext->height;

	int frameNumber = 0;

	// 设置 SWS 转换上下文
	SwsContext* swsContext = sws_getContext(
		codecContext->width, codecContext->height, codecContext->pix_fmt,
		codecContext->width, codecContext->height, AV_PIX_FMT_RGB32,
		SWS_BILINEAR, nullptr, nullptr, nullptr);

	int rgbBufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB32, codecContext->width, codecContext->height, 1);
	uint8_t* rgbBuffer = (uint8_t*)av_malloc(rgbBufferSize);
	av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, rgbBuffer, AV_PIX_FMT_RGB32, codecContext->width, codecContext->height, 1);


	// 读取数据包
	while (av_read_frame(formatContext, packet) >= 0) {
		if (packet->stream_index == videoStreamIndex) {
			// 判断是否为 I 帧
			if (/*packet->flags & AV_PKT_FLAG_KEY*/1) {
				if (avcodec_send_packet(codecContext, packet) == 0) {
					while (avcodec_receive_frame(codecContext, frame) == 0) {
                        if (frameNumber % 90 == 0) {
                            int ret = sws_scale(swsContext, frame->data, frame->linesize, 0, codecContext->height, rgbFrame->data, rgbFrame->linesize);
                            //std::cout << "Saving RGB frame " << frameNumber << " (PTS: " << frame->pts << ")" << std::endl;
                            AVFrame* dstFrame = av_frame_alloc();
                            dstFrame->format = AV_PIX_FMT_RGB32;
                            dstFrame->width = rgbFrame->width;
                            dstFrame->height = rgbFrame->height;
                            dstFrame->linesize[0] = rgbFrame->linesize[0];

                            if (av_frame_get_buffer(dstFrame, 0) < 0) {
                                av_frame_free(&dstFrame);
                                continue;
                            }

                            if (av_frame_copy(dstFrame, rgbFrame) < 0) {
                                av_frame_free(&dstFrame);
                                continue;
                            }
                            m_IFrame.push_back(dstFrame);
                        }
						frameNumber++;
					}
				}
			}
		}
		av_packet_unref(packet);
	}

	// 释放资源
	av_free(rgbBuffer);
	av_frame_free(&rgbFrame);
	av_frame_free(&frame);
	av_packet_free(&packet);
	sws_freeContext(swsContext);
	avcodec_free_context(&codecContext);
	avformat_close_input(&formatContext);
	return RET_OK;
}


// 初始化视频流
AVStream* AnalyzeFrameEngine::add_video_stream(AVFormatContext* fmt_ctx, AVCodecID codec_id, int width, int height, int fps) {
	AVCodec* codec = avcodec_find_encoder(codec_id);
	if (!codec) {
		std::cerr << "Video codec not found\n";
		return nullptr;
	}

	AVStream* stream = avformat_new_stream(fmt_ctx, codec);
	if (!stream) {
		std::cerr << "Could not allocate video stream\n";
		return nullptr;
	}

	AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
	if (!codec_ctx) {
		std::cerr << "Could not allocate video codec context\n";
		return nullptr;
	}

	stream->codecpar->codec_id = codec_id;
	stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	stream->codecpar->width = width;
	stream->codecpar->height = height;
	stream->codecpar->format = AV_PIX_FMT_YUV420P;
	stream->time_base = { 1, fps };

	codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	codec_ctx->codec_id = codec_id;
	codec_ctx->width = width;
	codec_ctx->height = height;
	codec_ctx->time_base = { 1, fps };
	codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
	codec_ctx->gop_size = fps; // Keyframe interval
	codec_ctx->max_b_frames = 0;

	if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	if (avcodec_parameters_from_context(stream->codecpar, codec_ctx) < 0) {
		std::cerr << "Failed to copy codec parameters\n";
		return nullptr;
	}

	if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
		std::cerr << "Could not open codec\n";
		return nullptr;
	}

	stream->codec = codec_ctx;
	return stream;
}

// 初始化音频流
AVStream* AnalyzeFrameEngine::add_audio_stream(AVFormatContext* fmt_ctx, AVCodecID codec_id, int sample_rate, int channels) {
	AVCodec* codec = avcodec_find_encoder(codec_id);
	if (!codec) {
		std::cerr << "Audio codec not found\n";
		return nullptr;
	}

	AVStream* stream = avformat_new_stream(fmt_ctx, codec);
	if (!stream) {
		std::cerr << "Could not allocate audio stream\n";
		return nullptr;
	}

	AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
	if (!codec_ctx) {
		std::cerr << "Could not allocate audio codec context\n";
		return nullptr;
	}

	stream->codecpar->codec_id = codec_id;
	stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
	stream->codecpar->sample_rate = sample_rate;
	stream->codecpar->channels = channels;
	stream->codecpar->channel_layout = av_get_default_channel_layout(channels);
	stream->codecpar->format = AV_SAMPLE_FMT_FLTP;
	stream->time_base = { 1, sample_rate };

	codec_ctx->codec_id = codec_id;
	codec_ctx->sample_rate = sample_rate;
	codec_ctx->channels = channels;
	codec_ctx->channel_layout = av_get_default_channel_layout(channels);
	codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
	codec_ctx->time_base = { 1, sample_rate };
	codec_ctx->frame_size = 1024;

	if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	if (avcodec_parameters_from_context(stream->codecpar, codec_ctx) < 0) {
		std::cerr << "Failed to copy codec parameters\n";
		return nullptr;
	}

	if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
		std::cerr << "Could not open codec\n";
		return nullptr;
	}

	stream->codec = codec_ctx;
	return stream;
}

void AnalyzeFrameEngine::read_thread() {
	while (m_isDone) {
		int ret;
		if (m_seek_req) { // 是否有seek请求
			int64_t seek_target = m_seek_pos;
			int64_t seek_min = m_seek_rel > 0 ? seek_target - m_seek_rel + 2 : INT64_MIN;
			int64_t seek_max = m_seek_rel < 0 ? seek_target = m_seek_rel - 2 : INT64_MAX;

			ret = avformat_seek_file(m_avformat_context, -1, seek_min, seek_target, seek_max, m_seek_flags);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "%s: error while seeking\n", m_avformat_context->url);
			}
			else {
				/* seek的时候，要把原先的数据清空，并重启解码器，
				 * put flush_pkt的目的是告知解码线程需要reset decoder
				 */
				if (m_audio_stream >= 0) {  // 如果有音频流
					m_avpacket_queue.packet_audio_queue_flash();
					m_avpacket_queue.audio_queue_put_flash();
				}
				if (m_video_stream >= 0) { // 如果有视频流
					m_avpacket_queue.packet_video_queue_flash();
					m_avpacket_queue.video_queue_put_flash();
				}
				m_seek_req = 0;
				//queue_attachments_req = 1; // 封面
				m_eof = 0;
			}
		}

		// 判断队列最大值,此处判断队列是否有足够的数据，进行休眠
		if (m_paused || m_avpacket_queue.get_video_packet_size() + m_avpacket_queue.get_audio_packet_size() > MAX_QUEUE_SIZE) {
			// wait 10 ms
			// seek 操作时会被唤醒
			std::unique_lock<std::mutex> lock(*m_wait_mutex);
			m_cond_t_read_thread->wait_for(lock, std::chrono::milliseconds(10));
			continue;		// 继续循环
		}

		// 检测码流是否已经播放结束
		//if(!m_paused && )

		//m_bReadFrame = true; // 打断堵塞，例如av_read_frame堵塞
		m_timeoutReadFrame = 0; // 打断堵塞，例如av_read_frame堵塞
		m_readframe_callback_time = av_gettime_relative();

		// 读取媒体数据
		ret = av_read_frame(m_avformat_context, m_avpacket); // 每一次 av_read_frame 都需要 av_packet_unref 释放内存

		// 检测数据是否读取完毕
		if (ret < 0) {
			//m_bReadFrame = false;
			if ((ret == AVERROR_EOF || avio_feof(m_avformat_context->pb)) && !m_eof) {
				// 插入空包说明码流数据读取完毕，为了从解码器把说有帧都读出来
				if (m_video_stream >= 0) {
					m_avpacket_queue.video_queue_put_nullpacket(m_video_stream);
				}
				if (m_audio_stream >= 0) {
					m_avpacket_queue.audio_queue_put_nullpacket(m_audio_stream);
				}
				m_eof = 1; // 文件读取完毕
			}
			if (m_avformat_context->pb && m_avformat_context->pb->error) {
				LogInfo("av_read_frame pb & error. errorcode: %d", m_avformat_context->pb->error);
				if (m_bReadFrame) { // 超时
					ONPLAYERSTATECHANGED_EVENT(PLAYER_STATE_FAILED, PLAYER_ERROR_READFRAME_TIMEOUT);
				}
				else { // 被打断阻塞 强制结束播放
					ONPLAYERSTATECHANGED_EVENT(PLAYER_STATE_PLAYBACK_COMPLETED, PLAYER_ERROR_NONE);
				}
				//m_isDone = false;
				break; // thread end
			}

			std::unique_lock<std::mutex> lock(*m_wait_mutex);
			m_cond_t_read_thread->wait_for(lock, std::chrono::milliseconds(10));

			auto audio_queue = m_audio_decode_thread->getAVPacketQueue();
			auto video_queue = m_video_decode_thread->getAVPacketQueue();
			if (video_queue->video_frame_queue_nb_remaining() <= 0 && audio_queue->audio_frame_queue_nb_remaining() <= 0)
			{
				// 文件播放完成
				ONPLAYERSTATECHANGED_EVENT(PLAYER_STATE_PLAYBACK_COMPLETED, PLAYER_ERROR_NONE);
				break;
			}

			continue; // 继续循环
		}
		//m_bReadFrame = false;

		if (m_avpacket->stream_index == m_video_stream) { // 视频流
			m_avpacket_queue.packet_video_queue_put(m_avpacket);
		}
		else if (m_avpacket->stream_index == m_audio_stream) { // 音频流
			m_avpacket_queue.packet_audio_queue_put(m_avpacket);
		}
		else {
			av_packet_unref(m_avpacket);
		}
	}
	LogInfo("av_read_frame thread end.");
}
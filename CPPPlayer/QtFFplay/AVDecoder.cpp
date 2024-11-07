#include "AVDecoder.h"

namespace cvpublish {

AVDecoder::AVDecoder(FFDecoder* ffmpegdecoder, AVPacketQueue* avpacketQueue, AVClock* avclock):
	m_decoder(ffmpegdecoder),
	m_avpacketqueue(avpacketQueue),
	m_avclock(avclock),
	m_codec_type(m_decoder->codec_context->codec_type)
{
	if (m_codec_type == AVMEDIA_TYPE_AUDIO) {
		// audio format
		m_resampler_params.src_sample_fmt = ffmpegdecoder->codec_context->sample_fmt;
		m_resampler_params.src_sample_rate = ffmpegdecoder->codec_context->sample_rate;
		m_resampler_params.src_channel_layout = ffmpegdecoder->codec_context->channel_layout;
		m_resampler_params.src_nb_channels = ffmpegdecoder->codec_context->channels;

		// 音频重采样设置 输出参数
		m_resampler_params.dst_channel_layout = AV_CH_LAYOUT_STEREO;                    // 双声道
		m_resampler_params.dst_sample_fmt = AV_SAMPLE_FMT_S16;                          // S16交错模式
		m_resampler_params.dst_sample_rate = ffmpegdecoder->codec_context->sample_rate; // 采样率
		m_resampler_params.dst_nb_samples = ffmpegdecoder->codec_context->frame_size;   // 需要的采样点数 MP3 1152 mp4 1024 rtmp 2048

		m_audioresample.set_samples_param(&m_resampler_params);

		// 1. 给采样器分配空间
		if (m_audioresample.audio_resampler_alloc() != 0) {
			av_log(NULL, AV_LOG_ERROR, "audio_resampler_alloc failed\n");
		}

		// 2. 给输出缓存分配空间
		if (m_audioresample.audio_destination_samples_alloc() <= 0) {
			av_log(NULL, AV_LOG_ERROR, "audio_resampler_alloc failed\n");
		}

		m_resampler_params.frame_size = av_samples_get_buffer_size(NULL, m_resampler_params.dst_nb_channels, 1, m_resampler_params.dst_sample_fmt, 1);
		m_resampler_params.bytes_per_sec = av_samples_get_buffer_size(NULL, m_resampler_params.dst_nb_channels,
			m_resampler_params.dst_sample_rate,
			m_resampler_params.dst_sample_fmt, 1);
	}
	else if (m_codec_type == AVMEDIA_TYPE_VIDEO) {
		m_avframe_yuv420 = av_frame_alloc();
		m_avframe_yuv420->width = 1280;
		m_avframe_yuv420->height = 720;

		//转换器上下文  转换方法需要
		m_img_convert_ctx = sws_getContext(m_decoder->codec_context->width, m_decoder->codec_context->height,
			m_decoder->codec_context->pix_fmt,
			//m_decoder->codec_context->width, m_decoder->codec_context->height, 
			m_avframe_yuv420->width, m_avframe_yuv420->height, // out width height
			AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);

		int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, m_avframe_yuv420->width, m_avframe_yuv420->height, 1);
		m_out_buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
		av_image_fill_arrays(m_avframe_yuv420->data, m_avframe_yuv420->linesize, m_out_buffer, AV_PIX_FMT_YUV420P, m_avframe_yuv420->width, m_avframe_yuv420->height, 1);
	}
}

AVDecoder::~AVDecoder(){
	if (m_audio_speed_convert) {
		sonicDestroyStream(m_audio_speed_convert);
	}
}

void AVDecoder::Pause() {
	m_paused = !m_paused;
}

int AVDecoder::decoder_frame() {
	int ret = AVERROR(EAGAIN);

	// 循环获取解码帧
	for (;;)
	{
		AVPacket pkt;
		if (m_decoder->queue->serial == m_decoder->pkt_serial) { // 判断是否是同一播放序列的数据, 用于seek
			do
			{
				switch (m_codec_type)
				{
				case AVMEDIA_TYPE_VIDEO:
				{
					// 1. 先获取解码帧
					ret = avcodec_receive_frame(m_decoder->codec_context, m_av_frame);
					if (ret >= 0) {
						// 设置pts
						m_av_frame->pts = m_av_frame->best_effort_timestamp;
					}
					break;
				}
				case AVMEDIA_TYPE_AUDIO:
				{
					// 1. 先获取解码帧
					ret = avcodec_receive_frame(m_decoder->codec_context, m_av_frame);
					if (ret >= 0) {
						// 设置pts
						AVRational tb = {1, m_av_frame->sample_rate};
						if (m_av_frame->pts != AV_NOPTS_VALUE) {
							// 时基转换
							// 如果m_av_frame->pts正常，则先将其从pkt_timebase转成{1, m_av_frame->sample_rate}
							// pkt_timebase实质就是stream->time_base
							m_av_frame->pts = av_rescale_q(m_av_frame->pts, m_decoder->codec_context->pkt_timebase, tb);
						}
						else if (m_decoder->next_pts != AV_NOPTS_VALUE) {
							// 如果m_av_frame->pts不正常则使用上一帧更新的next_pts和next_pts_tb
							// 转成{1, m_av_frame->sample_rate}
							m_av_frame->pts = av_rescale_q(m_decoder->next_pts, m_decoder->next_pts_tb, tb);
						}

						// 根据当前帧的pts和nb_samples预估下一帧的pts
						if (m_av_frame->pts != AV_NOPTS_VALUE) {
							m_decoder->next_pts = m_av_frame->pts + m_av_frame->nb_samples;
							m_decoder->next_pts_tb = tb;
						}
					}
					break;
				}
				default:
					break;
				}

				// 判断是否解码结束
				if (ret == AVERROR_EOF) {
					m_decoder->finished = m_decoder->pkt_serial;
					return 0;
				}

				// 正常解码返回1
				if (ret >= 0)
					return 1;
			} while (ret != AVERROR(EAGAIN));
		}

		// 2. 循环 获取packet
		for (;;) {
			switch (m_codec_type)
			{
			case AVMEDIA_TYPE_VIDEO:
			{
				ret = m_avpacketqueue->video_queue_get(&pkt, m_decoder->pkt_serial);
				break;
			}
			case AVMEDIA_TYPE_AUDIO:
			{
				ret = m_avpacketqueue->audio_queue_get(&pkt, m_decoder->pkt_serial);
				break;
			}
			}
		
			if (ret < 0)
				return -1;

			if (m_decoder->queue->serial == m_decoder->pkt_serial) // 如果是同一播放序列，此帧符合
				break;
		}

		// 3. 将packet送入解码器
		if (m_avpacketqueue->isNeedFlashbuffers(&pkt)) {
			avcodec_flush_buffers(m_decoder->codec_context);
			m_decoder->finished = 0;
			m_decoder->next_pts = m_decoder->start_pts;
			m_decoder->next_pts_tb = m_decoder->start_pts_tb;
		}
		else {
			if (m_decoder->codec_context->codec_type != AVMEDIA_TYPE_SUBTITLE) {
				// 音视频流处理
				if (avcodec_send_packet(m_decoder->codec_context, &pkt) == AVERROR(EAGAIN)) { // 放入packet失败，重新处理pending
					m_decoder->packet_pending = 1;
					av_packet_move_ref(&m_decoder->pkt, &pkt); // 将src中的每个字段移动到dst，并重置src。
				}
			}
			else {
				// ffplay.c 字幕流处理
			}
			av_packet_unref(&pkt);	// 一定要自己去释放音视频数据
		}
	}
}

void AVDecoder::Loop() {

	double      pts;  // pts
	int64_t		pos;  // 该帧在输入文件中的字节位置
	AVRational tb;    // 获取stream timebase
	int		serial;   // 帧序列，在seek的操作时serial会变化
	double		duration;       // 该帧持续时间，单位为秒

	m_av_frame = av_frame_alloc();  // 分配解码帧

	if (m_codec_type == AVMEDIA_TYPE_VIDEO) {
		LogInfo("vidio decode thread start");
		tb = m_decoder->video_st->time_base; // 获取stream timebase
	}
	else if (m_codec_type == AVMEDIA_TYPE_AUDIO) {
		LogInfo("audio decode thread start");
	}

	while (!request_exit_) {
		int ret = decoder_frame();
		if (ret < 0) {
			// 解码结束 
			goto end;
		}
		if(!ret)
			continue;

		if (m_codec_type == AVMEDIA_TYPE_VIDEO) {
			// 视频解码 计算pts
			//2 获取帧率，以便计算每帧picture的duration
			AVRational frame_rate = av_guess_frame_rate(m_decoder->avformat_context, m_decoder->video_st, NULL);
			// 4 计算帧持续时间和换算pts值为秒
			// 1/帧率 = duration 单位秒, 没有帧率时则设置为0, 有帧率帧计算出帧间隔
			duration = (frame_rate.num && frame_rate.den ? av_q2d({ frame_rate.den, frame_rate.num }) : 0);
			// 根据AVStream timebase计算出pts值, 单位为秒
			pts = (m_av_frame->pts == AV_NOPTS_VALUE) ? NAN : m_av_frame->pts * av_q2d(tb);
			// pos
			pos = m_av_frame->pkt_pos;

			// 解码后放到队列
			m_avpacketqueue->frame_video_frame_put(m_av_frame, pts, duration, pos, m_decoder->pkt_serial);
		}
		else if (m_codec_type == AVMEDIA_TYPE_AUDIO) {
			// 音频解码
			tb = { 1, m_av_frame->sample_rate };
			pts = (m_av_frame->pts == AV_NOPTS_VALUE) ? NAN : m_av_frame->pts * av_q2d(tb);
			duration = av_q2d({ m_av_frame->nb_samples, m_av_frame->sample_rate });
			pos = m_av_frame->pkt_pos;
			serial = m_decoder->pkt_serial;
			m_avpacketqueue->frame_audio_frame_put(m_av_frame, pts, duration, pos, m_decoder->pkt_serial);
		}

		// 释放数据
		av_frame_unref(m_av_frame);
	}
	LogInfo("audio video decode thread end");

end:
	//m_audio_buffer1 = nullptr;
	//if (m_audio_buffer1)
	//	av_freep(m_audio_buffer1);

	if(m_av_frame)
		av_frame_free(&m_av_frame);

	if(m_avframe_yuv420)
		av_frame_free(&m_avframe_yuv420);

	if (m_out_buffer)
		av_free(m_out_buffer);

	if(m_audio_speed_buf)
		av_free(m_audio_speed_buf);

	m_audio_buffer = NULL;

	m_audio_buffer1 = NULL;

	if (m_img_convert_ctx)
		sws_freeContext(m_img_convert_ctx);
}


int AVDecoder::getVideoYuv420Frame(uint8_t** y, uint8_t** u, uint8_t** v, int& width, int& height) {
	if (m_remaining_time > 0.0) {   //sleep控制画面输出的时机
		av_usleep((int64_t)(m_remaining_time * 1000000.0)); // remaining_time <= REFRESH_RATE 单位：微妙
		//av_log(NULL, AV_LOG_ERROR, "remaining_time sleep: %f\ns", m_remaining_time);
	}
	m_remaining_time = REFRESH_RATE;

	int ret = 1, got_picture = 1;
	Frame* sp;
	//do {
retry:
		if (m_avpacketqueue->video_frame_queue_nb_remaining() <= 0) // 判断队列是否为空
			return  -1;

		double last_duration, duration, delay, time;
		Frame *vp, *lastvp;

		lastvp = m_avpacketqueue->video_frame_queue_peek_last(); //读取上一帧
		vp = m_avpacketqueue->video_frame_queue_peek();  // 读取待显示帧

		// 如果不是最新的播放序列，则将其出队列，以尽快读取最新序列的帧
		if (vp->serial != m_avpacketqueue->get_video_packet_point()->serial) {
			m_avpacketqueue->video_frame_queue_next();
			goto retry;
		}

		// 新的播放序列 重置时间
		if (lastvp->serial != vp->serial) {
			// 新的播放序列重置当前时间
			m_frame_timer = av_gettime_relative() / 1000000.0;
		}

		// 暂停
		if (m_paused){
			goto display;
			//printf("视频暂停is->paused");
		}

		// 计算上一帧应显示的时长
		last_duration = vp_duration(m_avclock->max_frame_duration, lastvp, vp);

		// 计算delay 主要判断上一帧是否已经过了显示时间 返回的值越大，画面越慢
		delay = compute_target_delay(last_duration, *m_avclock);
		time = av_gettime_relative() / 1000000.0;
		if (time < m_frame_timer + delay) {  //判断是否继续显示上一帧
			// 继续显示上一帧
			//av_log(NULL, AV_LOG_ERROR, "continue lastvp frame! delay: %f\n", delay);
			m_remaining_time = FFMIN(m_frame_timer + delay - time, m_remaining_time);
			return -1;
		}

		// 走到这一步，说明已经到了或过了该显示的时间，待显示帧vp的状态变更为当前要显示的帧
		m_frame_timer += delay;   // 更新当前帧播放的时间
		if (delay > 0 && time - m_frame_timer > AV_SYNC_THRESHOLD_MAX) {
			m_frame_timer = time; //如果和系统时间差距太大，就纠正为系统时间
		}

		// 更新video时钟
		auto pictq = m_avpacketqueue->get_frame_video_queue();
		pictq->mutex_t->lock();
		if (!isnan(vp->pts))
			m_avclock->update_video_pts(vp->pts, vp->pos, m_pf_playback_rate, vp->serial);
		pictq->mutex_t->unlock();

		// 丢帧逻辑
		if (m_avpacketqueue->video_frame_queue_nb_remaining() > 1){ //有nextvp才会检测是否该丢帧
			Frame *nextvp = m_avpacketqueue->video_frame_queue_peek_next();
			duration = vp_duration(m_avclock->max_frame_duration, vp, nextvp);
			if (//!is->step        // 非逐帧模式才检测是否需要丢帧 is->step==1 为逐帧播放
				//&& (framedrop > 0 ||      // cpu解帧过慢
				//(framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)) // 非视频同步方式
				 time > m_frame_timer + duration // 确实落后了一帧数据
				) {
				printf("%s(%d) dif:%lfs, drop frame\n", __FUNCTION__, __LINE__, (m_frame_timer + duration) - time);
				m_frame_drops_late++;             // 统计丢帧情况
				m_avpacketqueue->video_frame_queue_next();       // 这里实现真正的丢帧
				//(这里不能直接while丢帧，因为很可能audio clock重新对时了，这样delay值需要重新计算)
				goto retry; //回到函数开始位置，继续重试
			}
		}

		// 获取下一帧
		//if (!(sp = m_avpacketqueue->video_frame_get())) {
		//	return -1;
		//}
		m_avpacketqueue->video_frame_queue_next();  // 当前vp帧出队列
	//} while (sp->serial != m_avpacketqueue->get_video_packet_point()->serial);

display:
	if (ret >= 0) {
			// 格式转换 
			if (got_picture) {
				sp = vp;
				sws_scale(m_img_convert_ctx, (const uint8_t * const*)sp->frame->data, sp->frame->linesize, 0,
					m_decoder->codec_context->height, m_avframe_yuv420->data, m_avframe_yuv420->linesize);

				*y = m_avframe_yuv420->data[0];
				*u = m_avframe_yuv420->data[1];
				*v = m_avframe_yuv420->data[2];

				width = m_avframe_yuv420->width;
				height = m_avframe_yuv420->height;

				m_video_index++;
			}
	}

end:
	return ret; // 需要获取设备支持最大缓冲区size 
}

int AVDecoder::getAudioFrame(uint8_t** data, size_t size) {
	int ret = 0, audio_size = 0, len1 = 0;
	m_audio_callback_time = av_gettime_relative();

	int len = size;
	uint8_t* audio_index = *data;

	while (len > 0) {
		// 判断数据是否完全拷贝到缓冲区
		if (m_audio_buffer_index >= m_audio_speed_size) {
			audio_size = get_audio_decode_frame();
			if (audio_size < 0) {
				m_audio_buffer = NULL;
				m_audio_speed_size = 512 / m_resampler_params.frame_size * m_resampler_params.frame_size;
			}
			else {
				m_audio_speed_size = audio_size;
			}
			m_audio_buffer_index = 0;

			// 初始化
			if (m_audio_speed_convert == nullptr) {
				m_audio_speed_convert = sonicCreateStream(m_resampler_params.dst_sample_rate, m_resampler_params.dst_nb_channels);
			}

			// 变速
			if (audio_size > 0 && m_need_change_rate) {
				m_need_change_rate = false;

				// 设置变速系数
				sonicSetSpeed(m_audio_speed_convert, m_pf_playback_rate);
				sonicSetPitch(m_audio_speed_convert, 1.0);
				sonicSetRate(m_audio_speed_convert, 1.0);

			}
			// 修改音量
			if (m_need_change_volume)
			{
				m_need_change_volume = false;
				// 设置音量
				sonicSetVolume(m_audio_speed_convert, m_pf_volume);
			}

			if ((!is_normal_playback_rate() || !is_normal_playback_volume()) && m_audio_buffer) {
				int actual_out_samples = m_dst_bufsize /
					(m_resampler_params.dst_nb_channels * av_get_bytes_per_sample(m_resampler_params.dst_sample_fmt));
				// 计算处理后的点数
				int out_ret = 0;
				int out_size = 0;
				int num_samples = 0;
				int sonic_samples = 0;
				if (m_resampler_params.dst_sample_fmt == AV_SAMPLE_FMT_FLT) {
					out_ret = sonicWriteFloatToStream(m_audio_speed_convert, (float *)m_audio_buffer1, actual_out_samples);
				}
				else if (m_resampler_params.dst_sample_fmt == AV_SAMPLE_FMT_S16) {
					out_ret = sonicWriteShortToStream(m_audio_speed_convert, (short *)m_audio_buffer1, actual_out_samples);
				}
				else {
					av_log(NULL, AV_LOG_ERROR, "sonic unspport ......\n");
				}
				num_samples = sonicSamplesAvailable(m_audio_speed_convert);
				// 2通道  目前只支持2通道的
				out_size = (num_samples)* av_get_bytes_per_sample(m_resampler_params.dst_sample_fmt) * m_resampler_params.dst_nb_channels;

				av_fast_malloc(&m_audio_speed_buf, &m_audio_speed1_size, out_size);
				if (out_ret)
				{
					// 从流中读取处理好的数据
					if (m_resampler_params.dst_sample_fmt == AV_SAMPLE_FMT_FLT) {
						sonic_samples = sonicReadFloatFromStream(m_audio_speed_convert,
							(float *)m_audio_speed_buf,
							num_samples);
					}
					else  if (m_resampler_params.dst_sample_fmt == AV_SAMPLE_FMT_S16) {
						sonic_samples = sonicReadShortFromStream(m_audio_speed_convert,
							(short *)m_audio_speed_buf,
							num_samples);
					}
					else {
						av_log(NULL, AV_LOG_ERROR, "sonic unspport ......\n");
					}
					m_audio_buffer = m_audio_speed_buf;
					m_audio_speed_size = sonic_samples * m_resampler_params.dst_nb_channels *av_get_bytes_per_sample(m_resampler_params.dst_sample_fmt);
					m_audio_buffer_index = 0;
				}
				else {
					av_log(NULL, AV_LOG_ERROR, "sonic out_ret fail...........\n");
				}
			}
		}
		if(m_audio_speed_size == 0)
			continue;

		len1 = m_audio_speed_size - m_audio_buffer_index;
		if (len1 > len)
			len1 = len;

		if (m_audio_buffer) {
			memcpy(audio_index, (uint8_t *)m_audio_buffer + m_audio_buffer_index, len1);
		}
		else {
			memset(audio_index, 0, len1); // 静音
		}
		len -= len1;
		audio_index += len1;
		m_audio_buffer_index += len1;
	}
	m_write_buf_size = m_audio_speed_size - m_audio_buffer_index;

end:
	// 设置音频pts
	video_set_clock_at(m_write_buf_size);
	return ret;
}

int AVDecoder::get_audio_decode_frame() {
	Frame* af;
	if (m_paused)
		return -1;

	do {
#if defined(_WIN32)
		while (m_avpacketqueue->audio_frame_queue_nb_remaining() == 0) {
			if ((av_gettime_relative() - m_audio_callback_time) > 1000000LL * m_dst_bufsize / m_resampler_params.bytes_per_sec / 2)
				return -1;
			av_usleep(1000);
		}
#endif

		if (!(af = m_avpacketqueue->audio_frame_get())) {
			return -1;
		}
		m_avpacketqueue->audio_frame_queue_next();
	} while (af->serial != m_avpacketqueue->get_audio_packet_point()->serial);

	int64_t out_pts = 0;

	// 4. 音频重采样
	int ret_size = m_audioresample.audio_resampler_send_frame(af->frame);
	if (ret_size <= 0) {
		printf("can't get %d samples, ret_size:%d, cur_size:%d\n", m_resampler_params.dst_nb_samples, ret_size, m_audioresample.audio_resampler_get_fifo_size());
	}
	ret_size = m_audioresample.audio_resampler_receive_frame(m_resampler_params.dst_data, m_resampler_params.dst_nb_samples, &out_pts);
	if (ret_size > 0) {
		// 获取给定音频属性参数所需的缓冲区大小
		m_dst_bufsize = av_samples_get_buffer_size(&m_resampler_params.dst_linesize, m_resampler_params.dst_nb_channels, ret_size, m_resampler_params.dst_sample_fmt, 1);
		if (m_dst_bufsize < 0) {
			fprintf(stderr, "Could not get sample buffer size\n");
			return -1;
		}
		//av_fast_malloc(&m_audio_buffer1, &m_audio_buf1_size, m_dst_bufsize); // 如果缓冲区足够大重用缓冲区，否则重新malloc
		//if (!m_audio_buffer1)
		//	return AVERROR(ENOMEM);

		m_audio_buffer1 = m_resampler_params.dst_data[0];
		m_audio_buffer = m_audio_buffer1;
		m_audio_index++;
	}

	/*update the audio clock with the pts*/
	if (!isnan(af->pts))
		m_avclock->audio_clock = af->pts + (double)af->frame->nb_samples / af->frame->sample_rate;
	else
		m_avclock->audio_clock = NAN;
	m_avclock->audio_clock_serial = af->serial;

	return m_dst_bufsize;
}

void AVDecoder::Clear() {
}

void AVDecoder::video_set_clock_at(int write_buf_size) {
	// 第三次调整pts 有多少数据还没播放出去(以字节为单位) 计算音频的实时pts，需要减去缓存的数据播放时长
	//set_clock_at(clock, audio_clock - (double)(8192 + 0) / bytes_per_sec);
	if (!isnan(m_avclock->audio_clock)) {
		double audio_clock = m_avclock->audio_clock / m_pf_playback_rate;
		m_avclock->set_clock_at(&m_avclock->audclk, 
			audio_clock - (double)(2 * m_dst_bufsize + write_buf_size) / m_resampler_params.bytes_per_sec,
			m_avclock->audio_clock_serial,
			m_audio_callback_time / 1000000.0);
	}
}

double AVDecoder::vp_duration(double max_frame_duration, Frame *vp, Frame *nextvp) {
	if (vp->serial == nextvp->serial) { // 同一播放序列，序列连续的情况下
		double duration = nextvp->pts - vp->pts;
		if (isnan(duration) // duration 数值异常
			|| duration <= 0    // pts值没有递增时
			|| duration > max_frame_duration    // 超过了最大帧范围
			) {
			return vp->duration / m_pf_playback_rate;	 /* 异常时以帧时间为基准(1秒/帧率) */
		}
		else {
			return duration / m_pf_playback_rate; //使用两帧pts差值计算duration，一般情况下也是走的这个分支
		}
	}
	else {        // 不同播放序列, 序列不连续则返回0
		return 0.0;
	}
}

double AVDecoder::compute_target_delay(double delay, AVClock& avclock) {
	double sync_threshold, diff = 0.0f;

	if (avclock.get_master_sync_type() != AV_SYNC_VIDEO_MASTER) {
		diff = avclock.get_clock(&avclock.vidclk) - avclock.get_master_clock();

		sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
		if (!isnan(diff) && fabs(diff) < avclock.max_frame_duration) { // diff在最大帧duration内
			if (diff <= -sync_threshold) {
				delay = FFMAX(0, delay + diff);
			}
			else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD) {
				// 视频超前
                //AV_SYNC_FRAMEDUP_THRESHOLD是0.1，此时如果delay>0.1, 如果2*delay时间就有点久
				delay = delay + diff; // 上一帧持续时间往大的方向去调整
				av_log(NULL, AV_LOG_INFO, "video: delay=%0.3f A-V=%f\n", delay, -diff);
			}
			else if (diff >= sync_threshold) {
				delay = 2 * delay;
			}
			else {

			}
		}
	}
	av_log(NULL, AV_LOG_TRACE, "video: delay=%0.3f A-V=%f\n",
		delay, -diff);

	return delay;
}

void AVDecoder::setPlaybackRate(float rate) 
{
	m_pf_playback_rate = rate;
	setNeedChangeRate(true);
}

void AVDecoder::setPlaybackVolume(float volume)
{
	m_pf_volume = volume;
	setNeedChangeVolume(true);
}

int AVDecoder::is_normal_playback_volume()
{
	if (m_pf_volume < 1.0f && m_pf_volume >= 0.00f)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

int AVDecoder::is_normal_playback_rate()
{
	if (m_pf_playback_rate > 0.99 && m_pf_playback_rate < 1.01)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

}
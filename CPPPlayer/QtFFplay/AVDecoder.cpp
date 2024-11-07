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

		// ��Ƶ�ز������� �������
		m_resampler_params.dst_channel_layout = AV_CH_LAYOUT_STEREO;                    // ˫����
		m_resampler_params.dst_sample_fmt = AV_SAMPLE_FMT_S16;                          // S16����ģʽ
		m_resampler_params.dst_sample_rate = ffmpegdecoder->codec_context->sample_rate; // ������
		m_resampler_params.dst_nb_samples = ffmpegdecoder->codec_context->frame_size;   // ��Ҫ�Ĳ������� MP3 1152 mp4 1024 rtmp 2048

		m_audioresample.set_samples_param(&m_resampler_params);

		// 1. ������������ռ�
		if (m_audioresample.audio_resampler_alloc() != 0) {
			av_log(NULL, AV_LOG_ERROR, "audio_resampler_alloc failed\n");
		}

		// 2. ������������ռ�
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

		//ת����������  ת��������Ҫ
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

	// ѭ����ȡ����֡
	for (;;)
	{
		AVPacket pkt;
		if (m_decoder->queue->serial == m_decoder->pkt_serial) { // �ж��Ƿ���ͬһ�������е�����, ����seek
			do
			{
				switch (m_codec_type)
				{
				case AVMEDIA_TYPE_VIDEO:
				{
					// 1. �Ȼ�ȡ����֡
					ret = avcodec_receive_frame(m_decoder->codec_context, m_av_frame);
					if (ret >= 0) {
						// ����pts
						m_av_frame->pts = m_av_frame->best_effort_timestamp;
					}
					break;
				}
				case AVMEDIA_TYPE_AUDIO:
				{
					// 1. �Ȼ�ȡ����֡
					ret = avcodec_receive_frame(m_decoder->codec_context, m_av_frame);
					if (ret >= 0) {
						// ����pts
						AVRational tb = {1, m_av_frame->sample_rate};
						if (m_av_frame->pts != AV_NOPTS_VALUE) {
							// ʱ��ת��
							// ���m_av_frame->pts���������Ƚ����pkt_timebaseת��{1, m_av_frame->sample_rate}
							// pkt_timebaseʵ�ʾ���stream->time_base
							m_av_frame->pts = av_rescale_q(m_av_frame->pts, m_decoder->codec_context->pkt_timebase, tb);
						}
						else if (m_decoder->next_pts != AV_NOPTS_VALUE) {
							// ���m_av_frame->pts��������ʹ����һ֡���µ�next_pts��next_pts_tb
							// ת��{1, m_av_frame->sample_rate}
							m_av_frame->pts = av_rescale_q(m_decoder->next_pts, m_decoder->next_pts_tb, tb);
						}

						// ���ݵ�ǰ֡��pts��nb_samplesԤ����һ֡��pts
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

				// �ж��Ƿ�������
				if (ret == AVERROR_EOF) {
					m_decoder->finished = m_decoder->pkt_serial;
					return 0;
				}

				// �������뷵��1
				if (ret >= 0)
					return 1;
			} while (ret != AVERROR(EAGAIN));
		}

		// 2. ѭ�� ��ȡpacket
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

			if (m_decoder->queue->serial == m_decoder->pkt_serial) // �����ͬһ�������У���֡����
				break;
		}

		// 3. ��packet���������
		if (m_avpacketqueue->isNeedFlashbuffers(&pkt)) {
			avcodec_flush_buffers(m_decoder->codec_context);
			m_decoder->finished = 0;
			m_decoder->next_pts = m_decoder->start_pts;
			m_decoder->next_pts_tb = m_decoder->start_pts_tb;
		}
		else {
			if (m_decoder->codec_context->codec_type != AVMEDIA_TYPE_SUBTITLE) {
				// ����Ƶ������
				if (avcodec_send_packet(m_decoder->codec_context, &pkt) == AVERROR(EAGAIN)) { // ����packetʧ�ܣ����´���pending
					m_decoder->packet_pending = 1;
					av_packet_move_ref(&m_decoder->pkt, &pkt); // ��src�е�ÿ���ֶ��ƶ���dst��������src��
				}
			}
			else {
				// ffplay.c ��Ļ������
			}
			av_packet_unref(&pkt);	// һ��Ҫ�Լ�ȥ�ͷ�����Ƶ����
		}
	}
}

void AVDecoder::Loop() {

	double      pts;  // pts
	int64_t		pos;  // ��֡�������ļ��е��ֽ�λ��
	AVRational tb;    // ��ȡstream timebase
	int		serial;   // ֡���У���seek�Ĳ���ʱserial��仯
	double		duration;       // ��֡����ʱ�䣬��λΪ��

	m_av_frame = av_frame_alloc();  // �������֡

	if (m_codec_type == AVMEDIA_TYPE_VIDEO) {
		LogInfo("vidio decode thread start");
		tb = m_decoder->video_st->time_base; // ��ȡstream timebase
	}
	else if (m_codec_type == AVMEDIA_TYPE_AUDIO) {
		LogInfo("audio decode thread start");
	}

	while (!request_exit_) {
		int ret = decoder_frame();
		if (ret < 0) {
			// ������� 
			goto end;
		}
		if(!ret)
			continue;

		if (m_codec_type == AVMEDIA_TYPE_VIDEO) {
			// ��Ƶ���� ����pts
			//2 ��ȡ֡�ʣ��Ա����ÿ֡picture��duration
			AVRational frame_rate = av_guess_frame_rate(m_decoder->avformat_context, m_decoder->video_st, NULL);
			// 4 ����֡����ʱ��ͻ���ptsֵΪ��
			// 1/֡�� = duration ��λ��, û��֡��ʱ������Ϊ0, ��֡��֡�����֡���
			duration = (frame_rate.num && frame_rate.den ? av_q2d({ frame_rate.den, frame_rate.num }) : 0);
			// ����AVStream timebase�����ptsֵ, ��λΪ��
			pts = (m_av_frame->pts == AV_NOPTS_VALUE) ? NAN : m_av_frame->pts * av_q2d(tb);
			// pos
			pos = m_av_frame->pkt_pos;

			// �����ŵ�����
			m_avpacketqueue->frame_video_frame_put(m_av_frame, pts, duration, pos, m_decoder->pkt_serial);
		}
		else if (m_codec_type == AVMEDIA_TYPE_AUDIO) {
			// ��Ƶ����
			tb = { 1, m_av_frame->sample_rate };
			pts = (m_av_frame->pts == AV_NOPTS_VALUE) ? NAN : m_av_frame->pts * av_q2d(tb);
			duration = av_q2d({ m_av_frame->nb_samples, m_av_frame->sample_rate });
			pos = m_av_frame->pkt_pos;
			serial = m_decoder->pkt_serial;
			m_avpacketqueue->frame_audio_frame_put(m_av_frame, pts, duration, pos, m_decoder->pkt_serial);
		}

		// �ͷ�����
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
	if (m_remaining_time > 0.0) {   //sleep���ƻ��������ʱ��
		av_usleep((int64_t)(m_remaining_time * 1000000.0)); // remaining_time <= REFRESH_RATE ��λ��΢��
		//av_log(NULL, AV_LOG_ERROR, "remaining_time sleep: %f\ns", m_remaining_time);
	}
	m_remaining_time = REFRESH_RATE;

	int ret = 1, got_picture = 1;
	Frame* sp;
	//do {
retry:
		if (m_avpacketqueue->video_frame_queue_nb_remaining() <= 0) // �ж϶����Ƿ�Ϊ��
			return  -1;

		double last_duration, duration, delay, time;
		Frame *vp, *lastvp;

		lastvp = m_avpacketqueue->video_frame_queue_peek_last(); //��ȡ��һ֡
		vp = m_avpacketqueue->video_frame_queue_peek();  // ��ȡ����ʾ֡

		// ����������µĲ������У���������У��Ծ����ȡ�������е�֡
		if (vp->serial != m_avpacketqueue->get_video_packet_point()->serial) {
			m_avpacketqueue->video_frame_queue_next();
			goto retry;
		}

		// �µĲ������� ����ʱ��
		if (lastvp->serial != vp->serial) {
			// �µĲ����������õ�ǰʱ��
			m_frame_timer = av_gettime_relative() / 1000000.0;
		}

		// ��ͣ
		if (m_paused){
			goto display;
			//printf("��Ƶ��ͣis->paused");
		}

		// ������һ֡Ӧ��ʾ��ʱ��
		last_duration = vp_duration(m_avclock->max_frame_duration, lastvp, vp);

		// ����delay ��Ҫ�ж���һ֡�Ƿ��Ѿ�������ʾʱ�� ���ص�ֵԽ�󣬻���Խ��
		delay = compute_target_delay(last_duration, *m_avclock);
		time = av_gettime_relative() / 1000000.0;
		if (time < m_frame_timer + delay) {  //�ж��Ƿ������ʾ��һ֡
			// ������ʾ��һ֡
			//av_log(NULL, AV_LOG_ERROR, "continue lastvp frame! delay: %f\n", delay);
			m_remaining_time = FFMIN(m_frame_timer + delay - time, m_remaining_time);
			return -1;
		}

		// �ߵ���һ����˵���Ѿ����˻���˸���ʾ��ʱ�䣬����ʾ֡vp��״̬���Ϊ��ǰҪ��ʾ��֡
		m_frame_timer += delay;   // ���µ�ǰ֡���ŵ�ʱ��
		if (delay > 0 && time - m_frame_timer > AV_SYNC_THRESHOLD_MAX) {
			m_frame_timer = time; //�����ϵͳʱ����̫�󣬾;���Ϊϵͳʱ��
		}

		// ����videoʱ��
		auto pictq = m_avpacketqueue->get_frame_video_queue();
		pictq->mutex_t->lock();
		if (!isnan(vp->pts))
			m_avclock->update_video_pts(vp->pts, vp->pos, m_pf_playback_rate, vp->serial);
		pictq->mutex_t->unlock();

		// ��֡�߼�
		if (m_avpacketqueue->video_frame_queue_nb_remaining() > 1){ //��nextvp�Ż����Ƿ�ö�֡
			Frame *nextvp = m_avpacketqueue->video_frame_queue_peek_next();
			duration = vp_duration(m_avclock->max_frame_duration, vp, nextvp);
			if (//!is->step        // ����֡ģʽ�ż���Ƿ���Ҫ��֡ is->step==1 Ϊ��֡����
				//&& (framedrop > 0 ||      // cpu��֡����
				//(framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)) // ����Ƶͬ����ʽ
				 time > m_frame_timer + duration // ȷʵ�����һ֡����
				) {
				printf("%s(%d) dif:%lfs, drop frame\n", __FUNCTION__, __LINE__, (m_frame_timer + duration) - time);
				m_frame_drops_late++;             // ͳ�ƶ�֡���
				m_avpacketqueue->video_frame_queue_next();       // ����ʵ�������Ķ�֡
				//(���ﲻ��ֱ��while��֡����Ϊ�ܿ���audio clock���¶�ʱ�ˣ�����delayֵ��Ҫ���¼���)
				goto retry; //�ص�������ʼλ�ã���������
			}
		}

		// ��ȡ��һ֡
		//if (!(sp = m_avpacketqueue->video_frame_get())) {
		//	return -1;
		//}
		m_avpacketqueue->video_frame_queue_next();  // ��ǰvp֡������
	//} while (sp->serial != m_avpacketqueue->get_video_packet_point()->serial);

display:
	if (ret >= 0) {
			// ��ʽת�� 
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
	return ret; // ��Ҫ��ȡ�豸֧����󻺳���size 
}

int AVDecoder::getAudioFrame(uint8_t** data, size_t size) {
	int ret = 0, audio_size = 0, len1 = 0;
	m_audio_callback_time = av_gettime_relative();

	int len = size;
	uint8_t* audio_index = *data;

	while (len > 0) {
		// �ж������Ƿ���ȫ������������
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

			// ��ʼ��
			if (m_audio_speed_convert == nullptr) {
				m_audio_speed_convert = sonicCreateStream(m_resampler_params.dst_sample_rate, m_resampler_params.dst_nb_channels);
			}

			// ����
			if (audio_size > 0 && m_need_change_rate) {
				m_need_change_rate = false;

				// ���ñ���ϵ��
				sonicSetSpeed(m_audio_speed_convert, m_pf_playback_rate);
				sonicSetPitch(m_audio_speed_convert, 1.0);
				sonicSetRate(m_audio_speed_convert, 1.0);

			}
			// �޸�����
			if (m_need_change_volume)
			{
				m_need_change_volume = false;
				// ��������
				sonicSetVolume(m_audio_speed_convert, m_pf_volume);
			}

			if ((!is_normal_playback_rate() || !is_normal_playback_volume()) && m_audio_buffer) {
				int actual_out_samples = m_dst_bufsize /
					(m_resampler_params.dst_nb_channels * av_get_bytes_per_sample(m_resampler_params.dst_sample_fmt));
				// ���㴦���ĵ���
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
				// 2ͨ��  Ŀǰֻ֧��2ͨ����
				out_size = (num_samples)* av_get_bytes_per_sample(m_resampler_params.dst_sample_fmt) * m_resampler_params.dst_nb_channels;

				av_fast_malloc(&m_audio_speed_buf, &m_audio_speed1_size, out_size);
				if (out_ret)
				{
					// �����ж�ȡ����õ�����
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
			memset(audio_index, 0, len1); // ����
		}
		len -= len1;
		audio_index += len1;
		m_audio_buffer_index += len1;
	}
	m_write_buf_size = m_audio_speed_size - m_audio_buffer_index;

end:
	// ������Ƶpts
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

	// 4. ��Ƶ�ز���
	int ret_size = m_audioresample.audio_resampler_send_frame(af->frame);
	if (ret_size <= 0) {
		printf("can't get %d samples, ret_size:%d, cur_size:%d\n", m_resampler_params.dst_nb_samples, ret_size, m_audioresample.audio_resampler_get_fifo_size());
	}
	ret_size = m_audioresample.audio_resampler_receive_frame(m_resampler_params.dst_data, m_resampler_params.dst_nb_samples, &out_pts);
	if (ret_size > 0) {
		// ��ȡ������Ƶ���Բ�������Ļ�������С
		m_dst_bufsize = av_samples_get_buffer_size(&m_resampler_params.dst_linesize, m_resampler_params.dst_nb_channels, ret_size, m_resampler_params.dst_sample_fmt, 1);
		if (m_dst_bufsize < 0) {
			fprintf(stderr, "Could not get sample buffer size\n");
			return -1;
		}
		//av_fast_malloc(&m_audio_buffer1, &m_audio_buf1_size, m_dst_bufsize); // ����������㹻�����û���������������malloc
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
	// �����ε���pts �ж������ݻ�û���ų�ȥ(���ֽ�Ϊ��λ) ������Ƶ��ʵʱpts����Ҫ��ȥ��������ݲ���ʱ��
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
	if (vp->serial == nextvp->serial) { // ͬһ�������У����������������
		double duration = nextvp->pts - vp->pts;
		if (isnan(duration) // duration ��ֵ�쳣
			|| duration <= 0    // ptsֵû�е���ʱ
			|| duration > max_frame_duration    // ���������֡��Χ
			) {
			return vp->duration / m_pf_playback_rate;	 /* �쳣ʱ��֡ʱ��Ϊ��׼(1��/֡��) */
		}
		else {
			return duration / m_pf_playback_rate; //ʹ����֡pts��ֵ����duration��һ�������Ҳ���ߵ������֧
		}
	}
	else {        // ��ͬ��������, ���в������򷵻�0
		return 0.0;
	}
}

double AVDecoder::compute_target_delay(double delay, AVClock& avclock) {
	double sync_threshold, diff = 0.0f;

	if (avclock.get_master_sync_type() != AV_SYNC_VIDEO_MASTER) {
		diff = avclock.get_clock(&avclock.vidclk) - avclock.get_master_clock();

		sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
		if (!isnan(diff) && fabs(diff) < avclock.max_frame_duration) { // diff�����֡duration��
			if (diff <= -sync_threshold) {
				delay = FFMAX(0, delay + diff);
			}
			else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD) {
				// ��Ƶ��ǰ
                //AV_SYNC_FRAMEDUP_THRESHOLD��0.1����ʱ���delay>0.1, ���2*delayʱ����е��
				delay = delay + diff; // ��һ֡����ʱ������ķ���ȥ����
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
#include "PlayAudio.h"
#include "MyDevice.h"
#include <QDateTime>

PlayAudio::PlayAudio(AVPlayer* avplayer, QObject *parent) :
	QObject(parent),
	m_avplayer(avplayer)
{

	if (m_avplayer != NULL) {
		// �ز������� �������
		auto audio_decoder = m_avplayer->getAudioDecoder();

		SDL_AudioSpec wanted_spec, spec;
		const char *env;
		static const int next_nb_channels[] = { 0, 0, 1, 6, 2, 6, 4, 6 };
		static const int next_sample_rates[] = { 0, 44100, 48000, 96000, 192000 };
		int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;

		int64_t wanted_channel_layout = AV_CH_LAYOUT_STEREO;                        // 
		int wanted_nb_channels        = 2;                                          // ˫ͨ��
		int wanted_sample_rate        = audio_decoder->codec_context->sample_rate;  // ������
		int wanted_samples            = audio_decoder->codec_context->frame_size;   // ������

		env = SDL_getenv("SDL_AUDIO_CHANNELS");
		if (env) {  // ���������������ã����ȴӻ�������ȡ������������������
			wanted_nb_channels = atoi(env);
			wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
		}
		if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
			wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
			wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
		}
		// ����channel_layout��ȡnb_channels�����������wanted_nb_channels��ƥ��ʱ���˴���������
		wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
		wanted_spec.channels = wanted_nb_channels;
		wanted_spec.freq = wanted_sample_rate;
		if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
			LogInfo("Invalid sample rate or channel count!\n");
			return;
		}
		while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq)
			next_sample_rate_idx--;  // �Ӳ������������ҵ���һ�������ڴ������wanted_sample_rate��ֵ
		// ��Ƶ������ʽ���������ͣ�planar��packed������һ��˫������Ƶ�ļ���һ�����������������L��һ�����������������R����
		// planar�洢��ʽ��(plane1)LLLLLLLL...LLLL (plane2)RRRRRRRR...RRRR
		// packed�洢��ʽ��(plane1)LRLRLRLR...........................LRLR
		// �������ֲ��������£���ϸ�ֶ��ֲ�����ʽ����AV_SAMPLE_FMT_S16��AV_SAMPLE_FMT_S16P�ȣ�
		// ע��SDL2.0Ŀǰ��֧��planar��ʽ
		// channel_layout��int64_t���ͣ���ʾ��Ƶ�������֣�ÿbit����һ���ض����������ο�channel_layout.h�еĶ��壬һĿ��Ȼ
		// ������(bits/��) = ������(Hz) * �������(bit) * ������
		wanted_spec.format = AUDIO_S16SYS;
		wanted_spec.silence = 0;
		/*
		 * һ�ζ�ȡ�೤������
		 * SDL_AUDIO_MAX_CALLBACKS_PER_SECһ�����ص�����������Ƶ���Ļص�
		 *  Audio buffer size in samples (power of 2)
		 */
		//wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
		wanted_spec.samples  = wanted_samples;     // ������ ��Ҫ������ƵԴ��õ�
		wanted_spec.callback = sdl_audio_callback; // ��ƵPullģʽ�ص�
		wanted_spec.userdata = this;
		// ����Ƶ�豸��������Ƶ�����̡߳������Ĳ�����wanted_spec��ʵ�ʵõ���Ӳ��������spec
	// 1) SDL�ṩ����ʹ��Ƶ�豸ȡ����Ƶ���ݷ�����
	//    a. push��SDL���ض���Ƶ�ʵ��ûص��������ڻص�������ȡ����Ƶ����
	//    b. pull���û��������ض���Ƶ�ʵ���SDL_QueueAudio()������Ƶ�豸�ṩ���ݡ��������wanted_spec.callback=NULL
	// 2) ��Ƶ�豸�򿪺󲥷ž������������ص�������SDL_PauseAudio(0)�������ص�����ʼ����������Ƶ
	// SDL_OpenAudioDevice()��һ������ΪNULLʱ���ȼ���SDL_OpenAudio()
		while (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
			av_log(NULL, AV_LOG_WARNING, "SDL_OpenAudio (%d channels, %d Hz): %s\n",
				wanted_spec.channels, wanted_spec.freq, SDL_GetError());
			wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
			if (!wanted_spec.channels) {
				wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
				wanted_spec.channels = wanted_nb_channels;
				if (!wanted_spec.freq) {
					LogInfo("No more combinations to try, audio open failed\n");
					return;
				}
			}
			wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
		}

		// ������Ƶ�豸��ʵ�ʲ�����������ʽ
		if (spec.format != AUDIO_S16SYS) {
			LogInfo("SDL advised audio format %d is not supported!\n", spec.format);
			return;
		}
		// ������Ƶ�豸��ʵ�ʲ�����������
		if (spec.channels != wanted_spec.channels) {
			wanted_channel_layout = av_get_default_channel_layout(spec.channels);
			if (!wanted_channel_layout) {
				LogInfo("SDL advised channel count %d is not supported!\n", spec.channels);
				return;
			}
		}
		// wanted_spec�������Ĳ�����spec��ʵ�ʵĲ�����wanted_spec��spec����SDL�еĽṹ��
		// �˴�audio_hw_params��FFmpeg�еĲ���������������ϼ�����ʹ��
		// audio_hw_params����Ĳ��������������ز�����ʱ��Ҫת�ɵĸ�ʽ��
		audio_hw_params.fmt = AV_SAMPLE_FMT_S16;
		audio_hw_params.freq = spec.freq;
		audio_hw_params.channel_layout = wanted_channel_layout;
		audio_hw_params.channels = spec.channels;
		/* audio_hw_params->frame_size����ֻ�Ǽ���һ��������ռ�õ��ֽ��� */
		audio_hw_params.frame_size = av_samples_get_buffer_size(NULL, audio_hw_params.channels,
			1, audio_hw_params.fmt, 1);
		audio_hw_params.bytes_per_sec = av_samples_get_buffer_size(NULL, audio_hw_params.channels,
			audio_hw_params.freq,
			audio_hw_params.fmt, 1);
		if (audio_hw_params.bytes_per_sec <= 0 || audio_hw_params.frame_size <= 0) {
			LogInfo("av_samples_get_buffer_size failed\n");
			return;
		}
	}
	SDL_PauseAudio(0); // ��ʼ����
}

PlayAudio::~PlayAudio()
{
	SDL_CloseAudio();
}

void PlayAudio::sdl_audio_callback(void *opaque, Uint8 *stream, int len) {
	PlayAudio *is = (PlayAudio *)opaque;

	/*static uint64_t time1 = 0;
	uint64_t time2 = QDateTime::currentDateTime().toMSecsSinceEpoch();
	if (time1 == 0) {
		time1 = time2;
	}
	else {
		printf("ʱ��� : %d\n", time2 - time1);
		time1 = time2;
	}*/

	auto decoderptr = is->m_avplayer->getAudioDecode();
	if (decoderptr == nullptr)
		return;
	int pcm_size = decoderptr->getAudioFrame(&stream, len);

}
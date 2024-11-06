#include "PlayAudio.h"
#include "MyDevice.h"
#include <QDateTime>

PlayAudio::PlayAudio(AVPlayer* avplayer, QObject *parent) :
	QObject(parent),
	m_avplayer(avplayer)
{

	if (m_avplayer != NULL) {
		// 重采样参数 输出参数
		auto audio_decoder = m_avplayer->getAudioDecoder();

		SDL_AudioSpec wanted_spec, spec;
		const char *env;
		static const int next_nb_channels[] = { 0, 0, 1, 6, 2, 6, 4, 6 };
		static const int next_sample_rates[] = { 0, 44100, 48000, 96000, 192000 };
		int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;

		int64_t wanted_channel_layout = AV_CH_LAYOUT_STEREO;                        // 
		int wanted_nb_channels        = 2;                                          // 双通道
		int wanted_sample_rate        = audio_decoder->codec_context->sample_rate;  // 采样率
		int wanted_samples            = audio_decoder->codec_context->frame_size;   // 样本数

		env = SDL_getenv("SDL_AUDIO_CHANNELS");
		if (env) {  // 若环境变量有设置，优先从环境变量取得声道数和声道布局
			wanted_nb_channels = atoi(env);
			wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
		}
		if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
			wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
			wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
		}
		// 根据channel_layout获取nb_channels，当传入参数wanted_nb_channels不匹配时，此处会作修正
		wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
		wanted_spec.channels = wanted_nb_channels;
		wanted_spec.freq = wanted_sample_rate;
		if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
			LogInfo("Invalid sample rate or channel count!\n");
			return;
		}
		while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq)
			next_sample_rate_idx--;  // 从采样率数组中找到第一个不大于传入参数wanted_sample_rate的值
		// 音频采样格式有两大类型：planar和packed，假设一个双声道音频文件，一个左声道采样点记作L，一个右声道采样点记作R，则：
		// planar存储格式：(plane1)LLLLLLLL...LLLL (plane2)RRRRRRRR...RRRR
		// packed存储格式：(plane1)LRLRLRLR...........................LRLR
		// 在这两种采样类型下，又细分多种采样格式，如AV_SAMPLE_FMT_S16、AV_SAMPLE_FMT_S16P等，
		// 注意SDL2.0目前不支持planar格式
		// channel_layout是int64_t类型，表示音频声道布局，每bit代表一个特定的声道，参考channel_layout.h中的定义，一目了然
		// 数据量(bits/秒) = 采样率(Hz) * 采样深度(bit) * 声道数
		wanted_spec.format = AUDIO_S16SYS;
		wanted_spec.silence = 0;
		/*
		 * 一次读取多长的数据
		 * SDL_AUDIO_MAX_CALLBACKS_PER_SEC一秒最多回调次数，避免频繁的回调
		 *  Audio buffer size in samples (power of 2)
		 */
		//wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
		wanted_spec.samples  = wanted_samples;     // 样本数 需要分析视频源后得到
		wanted_spec.callback = sdl_audio_callback; // 音频Pull模式回调
		wanted_spec.userdata = this;
		// 打开音频设备并创建音频处理线程。期望的参数是wanted_spec，实际得到的硬件参数是spec
	// 1) SDL提供两种使音频设备取得音频数据方法：
	//    a. push，SDL以特定的频率调用回调函数，在回调函数中取得音频数据
	//    b. pull，用户程序以特定的频率调用SDL_QueueAudio()，向音频设备提供数据。此种情况wanted_spec.callback=NULL
	// 2) 音频设备打开后播放静音，不启动回调，调用SDL_PauseAudio(0)后启动回调，开始正常播放音频
	// SDL_OpenAudioDevice()第一个参数为NULL时，等价于SDL_OpenAudio()
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

		// 检查打开音频设备的实际参数：采样格式
		if (spec.format != AUDIO_S16SYS) {
			LogInfo("SDL advised audio format %d is not supported!\n", spec.format);
			return;
		}
		// 检查打开音频设备的实际参数：声道数
		if (spec.channels != wanted_spec.channels) {
			wanted_channel_layout = av_get_default_channel_layout(spec.channels);
			if (!wanted_channel_layout) {
				LogInfo("SDL advised channel count %d is not supported!\n", spec.channels);
				return;
			}
		}
		// wanted_spec是期望的参数，spec是实际的参数，wanted_spec和spec都是SDL中的结构。
		// 此处audio_hw_params是FFmpeg中的参数，输出参数供上级函数使用
		// audio_hw_params保存的参数，就是在做重采样的时候要转成的格式。
		audio_hw_params.fmt = AV_SAMPLE_FMT_S16;
		audio_hw_params.freq = spec.freq;
		audio_hw_params.channel_layout = wanted_channel_layout;
		audio_hw_params.channels = spec.channels;
		/* audio_hw_params->frame_size这里只是计算一个采样点占用的字节数 */
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
	SDL_PauseAudio(0); // 开始播放
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
		printf("时间差 : %d\n", time2 - time1);
		time1 = time2;
	}*/

	auto decoderptr = is->m_avplayer->getAudioDecode();
	if (decoderptr == nullptr)
		return;
	int pcm_size = decoderptr->getAudioFrame(&stream, len);

}
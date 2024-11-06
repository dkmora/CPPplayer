#include "MyAudioResample.h"

AVFrame *alloc_out_frame(const int nb_samples, const audio_resampler_params_t *resampler_params)
{
	int ret;
	AVFrame * frame = av_frame_alloc();
	if (!frame)
	{
		return NULL;
	}
	frame->nb_samples = nb_samples;
	frame->channel_layout = resampler_params->dst_channel_layout;
	frame->format = resampler_params->dst_sample_fmt;
	frame->sample_rate = resampler_params->dst_sample_rate;
	ret = av_frame_get_buffer(frame, 0);
	if (ret < 0)
	{
		printf("cannot allocate audio data buffer\n");
		return NULL;
	}
	return frame;
}

AudioResample::AudioResample() {

}

AudioResample::~AudioResample() {
	free_alloc();
}

void AudioResample::set_samples_param(audio_resampler_params_t* resampler_param)
{ 
	m_resameler_params = resampler_param; 
}

int AudioResample::audio_resampler_alloc() {
	int ret = 0;

	if (!m_resameler_params)
		return -1;

	// 设置通道数量
	m_src_channels = av_get_channel_layout_nb_channels(m_resameler_params->src_channel_layout);
	m_dst_channels = av_get_channel_layout_nb_channels(m_resameler_params->dst_channel_layout);
	// 分配audio fifo，单位为samples， 在av_audio_fifo_write时候如果buffer不足能自动扩充
	m_audio_fifo = av_audio_fifo_alloc(m_resameler_params->dst_sample_fmt, m_dst_channels, 1);

	if (!m_audio_fifo) {
		std::cout << "av_audio_fifo_alloc failed" << std::endl;
		return -1;
	}

	// 检查是否需要做重采样
	if (is_need_resampler()){
		std::cout << "no resample needed, just use audio fifo" << std::endl;
		m_is_fifo_only = 1;      // 不需要做重采样
		return -1;
	}

	// 初始化重采样
	m_swr_ctx = swr_alloc();
	if (!m_swr_ctx) {
		std::cout << "swr_alloc failed" << std::endl;
		return -1;
	}

	/* set options */
	av_opt_set_sample_fmt(m_swr_ctx, "in_sample_fmt",      m_resameler_params->src_sample_fmt, 0);
	av_opt_set_int(m_swr_ctx,        "in_channel_layout",  m_resameler_params->src_channel_layout, 0);
	av_opt_set_int(m_swr_ctx,        "in_sample_rate",     m_resameler_params->src_sample_rate, 0);
	av_opt_set_sample_fmt(m_swr_ctx, "out_sample_fmt",     m_resameler_params->dst_sample_fmt, 0);
	av_opt_set_int(m_swr_ctx,        "out_channel_layout", m_resameler_params->dst_channel_layout, 0);
	av_opt_set_int(m_swr_ctx,        "out_sample_rate",    m_resameler_params->dst_sample_rate, 0);

	/* initialize the resampling context */
	ret = swr_init(m_swr_ctx);
	if (ret < 0) {
		std::cout << "failed to initialize the resampling context." << std::endl;
		av_fifo_freep((AVFifoBuffer **)m_audio_fifo);
		swr_free(&m_swr_ctx);
		return -1;
	}

	m_resampled_data_size = 2048;
	if (init_resampled_data() < 0) {
		swr_free(&m_swr_ctx);
		av_fifo_freep((AVFifoBuffer **)m_audio_fifo);
		return -1;
	}

	std::cout << "init done" << std::endl;
	return 0;
}

bool AudioResample::is_need_resampler() {
	if (!m_resameler_params)
		return -1;

	return (m_resameler_params->src_sample_fmt == m_resameler_params->dst_sample_fmt &&
		m_resameler_params->src_sample_rate == m_resameler_params->dst_sample_rate &&
		m_resameler_params->src_channel_layout == m_resameler_params->dst_channel_layout);
}

int AudioResample::audio_source_samples_alloc() {
	int ret = -1;

	if (!m_resameler_params)
		return ret;

	m_resameler_params->src_nb_channels = av_get_channel_layout_nb_channels(m_resameler_params->src_channel_layout);

	ret = av_samples_alloc_array_and_samples(&m_resameler_params->src_data,
		&m_resameler_params->src_linesize,
		m_resameler_params->src_nb_channels, 
		m_resameler_params->src_nb_samples, 
		m_resameler_params->src_sample_fmt, 
		0);

	if (ret < 0) {
		free_alloc();
	}

	return ret; 
}

int AudioResample::audio_destination_samples_alloc() {
	int ret = -1;

	if (!m_resameler_params)
		return ret;

	if (m_resameler_params->dst_data != NULL)
		return 1;

	m_resameler_params->dst_nb_channels = av_get_channel_layout_nb_channels(m_resameler_params->dst_channel_layout);

	ret = av_samples_alloc_array_and_samples(&m_resameler_params->dst_data,
		&m_resameler_params->dst_linesize,
		m_resameler_params->dst_nb_channels,
		m_resameler_params->dst_nb_samples,
		m_resameler_params->dst_sample_fmt,
		0);

	if (ret < 0) {
		free_alloc();
	}

	return ret;
}

void AudioResample::free_alloc() {
	if (m_dst_file) 
		fclose(m_dst_file);

	if (m_resameler_params) {
		if (m_resameler_params->src_data) 
			av_freep(m_resameler_params->src_data[0]);
		av_freep(&m_resameler_params->src_data);

		if (m_resameler_params->src_data) 
			av_freep(&m_resameler_params->dst_data[0]);
		av_freep(&m_resameler_params->dst_data);
	}

	if (m_swr_ctx)
		swr_free(&m_swr_ctx);

	if (m_audio_fifo)
		av_fifo_freep((AVFifoBuffer **)m_audio_fifo);

	if (m_resameler_data)
		av_freep(&m_resameler_data[0]);
	av_freep(&m_resameler_data);

	if (m_av_format_context) {
		avformat_free_context(m_av_format_context);
		m_av_format_context = NULL;
	}

	if (m_av_codec_context) {
		avcodec_free_context(&m_av_codec_context);
		m_av_codec_context = NULL;
	}
}

int AudioResample::do_audio_resampler(uint8_t **in_data, uint8_t **out_data) {
	int ret_size = -1;
	ret_size = audio_resampler_send_frame(in_data,
		m_resameler_params->src_nb_samples,
		m_in_pts);
	m_in_pts += m_resameler_params->src_nb_samples;

	// 获取采样后音频帧
	ret_size = audio_resampler_receive_frame(out_data,
		m_resameler_params->dst_nb_samples,
		&m_out_pts);
	

	if (ret_size > 0 && m_dst_filename && m_dst_file) {  // 写到文件
		int dst_linesize = 0;
		int dst_bufsize = av_samples_get_buffer_size(&dst_linesize, m_resameler_params->dst_nb_channels,
			ret_size, m_resameler_params->dst_sample_fmt, 1);

		if (dst_bufsize < 0) {
			fprintf(stderr, "Could not get sample buffer size\n");
			free_alloc();
			return dst_bufsize;
		}
		//printf("t:%f in:%d out:%d, out_pts:%"PRId64"\n", t, src_nb_samples, ret_size, out_pts);
		fwrite(out_data[0], 1, dst_bufsize, m_dst_file);
	}
	else {
		write_file_error(ret_size);
	}

	return ret_size;
}

int AudioResample::audio_resampler_send_frame(AVFrame* frame) {
	if (!m_swr_ctx) {
		return 0;
	}

	int src_nb_samples = 0;
	uint8_t** src_data = NULL;
	if (frame) 
	{
		src_nb_samples = frame->nb_samples;
		src_data = frame->extended_data;
		if(m_start_pts == AV_NOPTS_VALUE && frame->pts != AV_NOPTS_VALUE)
		{
			m_start_pts = frame->pts;
			m_cur_pts = frame->pts;
		}
	}
	else
	{
		m_is_flushed = 1;
	}

	if (m_is_fifo_only) {
		// 如果不需要做重采样，原封不动写入fifo
		return src_data ? av_audio_fifo_write(m_audio_fifo, (void **)src_data, src_nb_samples) : 0;
	}

	// 计算这次做重采样能够获取到的重采样后的点
	const int dst_nb_resamples = av_rescale_rnd(swr_get_delay(m_swr_ctx, m_resameler_params->src_sample_rate) + src_nb_samples,
		m_resameler_params->src_sample_rate,
		m_resameler_params->dst_sample_rate,
		AV_ROUND_UP);

	if (dst_nb_resamples > m_resampled_data_size)
	{
		m_resampled_data_size = dst_nb_resamples;
		if (init_resampled_data() < 0) {
			return AVERROR(ENOMEM);
		}
	}

	int nb_samples = swr_convert(m_swr_ctx, m_resameler_data, dst_nb_resamples, (const uint8_t** )src_data, src_nb_samples);

	// 返回实际写入的采样点数量
	int ret_size = av_audio_fifo_write(m_audio_fifo, (void **)m_resameler_data, nb_samples);
	if (ret_size != nb_samples) {
		printf("Warn：av_audio_fifo_write failed, expected_write:%d, actual_write:%d\n", nb_samples, ret_size);
	}
	return ret_size;
}

int AudioResample::audio_resampler_send_frame(uint8_t** in_data, int in_nb_samples, int64_t pts) {
	if (!m_swr_ctx) {
		return 0;
	}

	int src_nb_samples = 0;
	uint8_t** src_data = NULL;
	if (in_data)
	{
		src_nb_samples = in_nb_samples;
		src_data = in_data;
		if (m_start_pts == AV_NOPTS_VALUE && pts != AV_NOPTS_VALUE)
		{
			m_start_pts = pts;
			m_cur_pts = pts;
		}
	}
	else
	{
		m_is_flushed = 1;
	}

	if (m_is_fifo_only) {
		return src_data ? av_audio_fifo_write(m_audio_fifo, (void **)src_data, src_nb_samples) : 0;
	}

	// 计算这次做重采样能够获取到的重采样后的点
	const int dst_nb_resamples = av_rescale_rnd(swr_get_delay(m_swr_ctx, m_resameler_params->src_sample_rate) + src_nb_samples,
		m_resameler_params->src_sample_rate,
		m_resameler_params->dst_sample_rate,
		AV_ROUND_UP);

	if (dst_nb_resamples > m_resampled_data_size)
	{
		m_resampled_data_size = dst_nb_resamples;
		if (init_resampled_data() < 0)
			return AVERROR(ENOMEM);
	}

	int nb_samples = swr_convert(m_swr_ctx, m_resameler_data, dst_nb_resamples, 
		(const uint8_t **)src_data, src_nb_samples);

	// 返回实际写入的采样点数量
	return av_audio_fifo_write(m_audio_fifo, (void **)m_resameler_data, nb_samples);
}

int AudioResample::audio_resampler_send_frame_byte(uint8_t* in_data, int in_bytes, int64_t pts){
	if (!m_swr_ctx) {
		return 0;
	}

	AVFrame* frame = NULL;
	if (in_data)
	{
		frame = av_frame_alloc();
		frame->format         = m_resameler_params->src_sample_fmt;
		frame->channel_layout = m_resameler_params->src_channel_layout;

		int ch = av_get_channel_layout_nb_channels(m_resameler_params->src_channel_layout);

		// 计算重采样的采样率
		frame->nb_samples = in_bytes / av_get_bytes_per_sample(m_resameler_params->src_sample_fmt) / ch;

		avcodec_fill_audio_frame(frame, ch, m_resameler_params->src_sample_fmt, in_data, in_bytes, 0);
		frame->pts = pts;
	}

	int ret = audio_resampler_send_frame(frame);
	if (frame) {
		av_frame_free(&frame);
	}
	return ret;
}

AVFrame* AudioResample::audio_resampler_receive_frame(int nb_samples) {
	nb_samples = nb_samples == 0 ? av_audio_fifo_size(m_audio_fifo) : nb_samples;
	if (av_audio_fifo_size(m_audio_fifo) < nb_samples || nb_samples == 0)
		return NULL;

	// 采样点数满足条件
	return get_one_frame(nb_samples);
}

int AudioResample::audio_resampler_receive_frame(uint8_t **out_data, int nb_samples, int64_t *pts) {
	nb_samples = nb_samples == 0 ? av_audio_fifo_size(m_audio_fifo) : nb_samples;

	int infifo_size = av_audio_fifo_size(m_audio_fifo);
	if (infifo_size < nb_samples || nb_samples == 0)
		return 0;
	int ret = av_audio_fifo_read(m_audio_fifo, (void **)out_data, nb_samples);
	*pts = m_cur_pts;
	m_cur_pts += nb_samples;
	m_total_resampled_num += nb_samples;
	return ret;
}

int AudioResample::audio_resampler_flush(uint8_t **out_data) {
	// flush
	audio_resampler_send_frame(NULL, 0, 0);
	int fifo_size = audio_resampler_get_fifo_size();
	int get_size = (fifo_size > m_resameler_params->dst_nb_samples ? m_resameler_params->dst_nb_samples : fifo_size);
	int ret_size = audio_resampler_receive_frame(out_data, get_size, &m_out_pts);

	if (ret_size > 0 && m_dst_filename && m_dst_file) {
		printf("flush ret_size:%d\n", ret_size);
		// 不够一帧的时候填充为静音, 这里的目的是补偿最后一帧如果不够采样点数不足1152，用静音数据进行补足
		av_samples_set_silence(out_data, 
			ret_size, 
			m_resameler_params->dst_nb_samples - ret_size, 
			m_resameler_params->dst_nb_channels, 
			m_resameler_params->dst_sample_fmt);

		int dst_linesize = 0;
		int dst_bufsize = av_samples_get_buffer_size(&dst_linesize, 
			m_resameler_params->dst_nb_channels,
			m_resameler_params->dst_nb_samples, 
			m_resameler_params->dst_sample_fmt, 
			1);

		if (dst_bufsize < 0) {
			fprintf(stderr, "Could not get sample buffer size\n");
			free_alloc();
			return dst_bufsize;
		}
		fwrite(out_data[0], 1, dst_bufsize, m_dst_file);
	}
	else {
		write_file_error(ret_size);
	}
}

int AudioResample::audio_resampler_get_fifo_size()
{
	return av_audio_fifo_size(m_audio_fifo);   // 获取fifo的采样点数量
}

int64_t AudioResample::audio_resampler_get_start_pts()
{
	return m_start_pts;
}

int64_t AudioResample::audio_resampler_get_cur_pts()
{
	return m_cur_pts;
}

int AudioResample::open_dst_file(std::string filename) {
	m_dst_filename = filename.c_str();

	m_dst_file = fopen(m_dst_filename, "wb");
	if (!m_dst_file) {
		fprintf(stderr, "Could not open destination file %s\n", m_dst_filename);
		return -1;
	}
	return 1;
}

int AudioResample::open_audio_info_codec(std::string filename) {
	m_src_filename = filename.c_str();

	m_av_format_context = avformat_alloc_context();
	int ret = avformat_open_input(&m_av_format_context, m_src_filename, NULL, NULL);
	if (ret != 0) {
		std::cout << "can't open src file:" << std::endl;
		return ret;
	}

	// print audio info
	av_dump_format(m_av_format_context, 0, m_src_filename, 0);

	// get the information of streams in the file
	ret = avformat_find_stream_info(m_av_format_context, NULL);
	if (ret < 0) {
		std::cout << "can't open src file:" << std::endl;
		return ret;
	}

	// find the index of audio in the stream array
	for (unsigned i = 0; i < m_av_format_context->nb_streams; ++i) {
		enum AVMediaType av_media_type = m_av_format_context->streams[i]->codecpar->codec_type;
		if (av_media_type == AVMEDIA_TYPE_AUDIO) {
			m_stream_index = i;
			break;
		}
	}

	// get input audio file info
	AVCodecParameters* av_codec_parameters = m_av_format_context->streams[m_stream_index]->codecpar;
	enum AVCodecID av_codec_id = av_codec_parameters->codec_id;
	AVCodec* av_codec = avcodec_find_decoder(av_codec_id);

	// create a AVCodecContext
	m_av_codec_context = avcodec_alloc_context3(av_codec);
	if (m_av_codec_context == NULL) {
		return -1;
	}

	// transform avCodecParameters to avCodecContext
	ret = avcodec_parameters_to_context(m_av_codec_context, av_codec_parameters);
	if (ret < 0) {
		std::cout << "parameters to context fail!" << std::endl;
		return ret;
	}

	ret = avcodec_open2(m_av_codec_context, av_codec, NULL);
	if (ret < 0) {
		std::cout << "can not open avcodec " << std::endl;
		return ret;
	}
	std::cout << "decodec name:" << av_codec->name;

	// audio format
	m_resameler_params->src_sample_fmt = m_av_codec_context->sample_fmt;
	m_resameler_params->src_sample_rate = m_av_codec_context->sample_rate;
	m_resameler_params->src_channel_layout = m_av_codec_context->channel_layout;

	return ret;
}

int AudioResample::get_audio_frame(AVFrame* frame) {

	// save input packet data before decode
	AVPacket *packet = (AVPacket* )av_malloc(sizeof(AVPacket));
	av_init_packet(packet);

	int ret = -1;
	if(av_read_frame(m_av_format_context, packet) >= 0){

		if (packet->stream_index == m_stream_index) {

			//decoding
			avcodec_send_packet(m_av_codec_context, packet);
			ret = avcodec_receive_frame(m_av_codec_context, frame);
			goto end;
		}
	}

end:
	av_packet_free(&packet);
	return ret;
}

AVFrame* AudioResample::get_one_frame(const int nb_samples) {
	AVFrame* frame = alloc_out_frame(nb_samples, m_resameler_params);
	if (frame)
	{
		av_audio_fifo_read(m_audio_fifo, (void* *)frame->data, nb_samples);
		frame->pts = m_cur_pts;
		m_cur_pts += nb_samples;
		m_total_resampled_num += nb_samples;
	}
	return frame;
}

AVFrame* AudioResample::alloc_out_frame(const int nb_samples, const audio_resampler_params_t *resampler_params)
{
	int ret;
	AVFrame * frame = av_frame_alloc();
	if (!frame)
	{
		return NULL;
	}
	frame->nb_samples = nb_samples;
	frame->channel_layout = resampler_params->dst_channel_layout;
	frame->format = resampler_params->dst_sample_fmt;
	frame->sample_rate = resampler_params->dst_sample_rate;
	ret = av_frame_get_buffer(frame, 0);
	if (ret < 0)
	{
		printf("cannot allocate audio data buffer\n");
		return NULL;
	}
	return frame;
}

int AudioResample::init_resampled_data() {
	if (m_resameler_data) {
		av_free(&m_resameler_data[0]);
		av_free(&m_resameler_data);
	}

	int linesize = 0;
	int ret = av_samples_alloc_array_and_samples(&m_resameler_data,
		&linesize,
		m_dst_channels,
		m_resampled_data_size,
		m_resameler_params->dst_sample_fmt,
		0);

	if (ret < 0) {
		std::cout << "fail accocate audio resampled data buffer" << std::endl;
	}
	return ret;
}

void AudioResample::write_file_error(int ret_size) {
	if (m_dst_filename && m_dst_file) {
		printf("can't get %d samples, ret_size:%d, cur_size:%d\n",
			m_resameler_params->dst_nb_samples,
			ret_size,
			audio_resampler_get_fifo_size());
	}
	else {
		printf("not open dst file\n");
	}
}
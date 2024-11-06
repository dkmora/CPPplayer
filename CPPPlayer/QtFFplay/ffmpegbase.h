#pragma once

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/pixfmt.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"
#include "libavutil/log.h"
#include "libavutil/time.h"
#include "libavutil/common.h"
}

#include <mutex>
#include <condition_variable>

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define VIDEO_PICTURE_QUEUE_SIZE	3       // ͼ��֡��������
#define SUBPICTURE_QUEUE_SIZE		16      // ��Ļ֡��������
#define SAMPLE_QUEUE_SIZE           9       // ����֡��������
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))
/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30


/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04  // 40ms
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1   // 100ms
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.040
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0
/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10
/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE 0.01
/**
 *����Ƶͬ����ʽ��ȱʡ����ƵΪ��׼
 */
enum {
	AV_SYNC_AUDIO_MASTER,                   // ����ƵΪ��׼
	AV_SYNC_VIDEO_MASTER,                   // ����ƵΪ��׼
	AV_SYNC_EXTERNAL_CLOCK,                 // ���ⲿʱ��Ϊ��׼��synchronize to an external clock */
};

// AVPacket����
typedef struct MyAVPacketList {
	AVPacket		pkt;    //���װ�������,av_read_frame��ȡ��AVPacket��ǳ����
	struct MyAVPacketList	*next;  //��һ���ڵ�
	int			serial = 0;     //��������
} MyAVPacketList;

typedef struct PacketQueue {
	MyAVPacketList	*first_pkt = nullptr, *last_pkt = nullptr;  // ���ף���βָ��
	int		nb_packets = 0;      // ��������Ҳ���Ƕ���Ԫ������
	int		size = 0;            // ��������Ԫ�ص����ݴ�С�ܺ�
	int64_t		duration = 0;    // ��������Ԫ�ص����ݲ��ų���ʱ��
	int		abort_request = 0;   // �û��˳������־
	int		serial = 0;          // �������кţ���MyAVPacketList��serial������ͬ�����ı��ʱ����΢�е㲻ͬ
	std::mutex* mutex_t = nullptr;       // ����ά��PacketQueue�Ķ��̰߳�ȫ(���԰�pthread_mutex_t��⣩
	std::condition_variable* cond_t = nullptr;      // ���ڶ���д�߳��໥֪ͨ(���԰�pthread_cond_t���)
} PacketQueue;

typedef struct AudioParams {
	int			freq;                   // ������
	int			channels;               // ͨ����
	int64_t		channel_layout;         // ͨ�����֣�����2.1������5.1������
	enum AVSampleFormat	fmt;            // ��Ƶ������ʽ������AV_SAMPLE_FMT_S16��ʾΪ�з���16bit��ȣ���������ģʽ��
	int			frame_size;             // һ��������Ԫռ�õ��ֽ���������2ͨ��ʱ��������ͨ��������һ�κϳ�һ��������Ԫ��
	int			bytes_per_sec;          // һ��ʱ����ֽ��������������48Khz��2 channel��16bit����һ��48000*2*16/8=192000
} AudioParams;

// ffmpeg ������
typedef struct FFDecoder {
	AVStream *video_st = nullptr;             // ��Ƶ��

	//
	AVPacket pkt;
	PacketQueue	*queue = nullptr;                  // ���ݰ�����
	AVFormatContext* avformat_context = nullptr;   // iformat��������
	AVCodecContext* codec_context = nullptr;       // ������������
	AVCodec* avcodec = nullptr;                    // ������
	int		pkt_serial = 0;         // ������
	int		finished = 0;           // =0�����������ڹ���״̬��=��0�����������ڿ���״̬
	int		packet_pending = 0;     // =0�������������쳣״̬����Ҫ�������ý�������=1����������������״̬
	std::condition_variable	*empty_queue_cond = nullptr;  // ��鵽packet���п�ʱ���� signal����read_thread��ȡ����
	int64_t		start_pts = 0;          // ��ʼ��ʱ��stream��start time
	AVRational	start_pts_tb;       // ��ʼ��ʱ��stream��time_base
	int64_t		next_pts = 0;           // ��¼���һ�ν�����frame��pts����������Ĳ���֡û����Ч��ptsʱ��ʹ��next_pts��������
	AVRational	next_pts_tb;        // next_pts�ĵ�λ
}FFDecoder;

// ���ڻ������������
typedef struct Frame {
	AVFrame		*frame;         // ָ������֡
	AVSubtitle	sub;            // ������Ļ
	int		serial;             // ֡���У���seek�Ĳ���ʱserial��仯
	double		pts;            // ʱ�������λΪ��
	double		duration;       // ��֡����ʱ�䣬��λΪ��
	int64_t		pos;            // ��֡�������ļ��е��ֽ�λ��
	int		width;              // ͼ����
	int		height;             // ͼ��߶�
	int		format;             // ����ͼ��Ϊ(enum AVPixelFormat)��
	// ����������Ϊ(enum AVSampleFormat)
	AVRational	sar;            // ͼ��Ŀ�߱ȣ�16:9��4:3...�������δ֪��δָ����Ϊ0/1
	int		uploaded;           // ������¼��֡�Ƿ��Ѿ���ʾ����
	int		flip_v;             // =1��ֱ��ת�� = 0����������
} Frame;

/* ����һ��ѭ�����У�windex��ָ���е���Ԫ�أ�rindex��ָ���е�β��Ԫ��. */
typedef struct FrameQueue {
	Frame	queue[FRAME_QUEUE_SIZE];        // FRAME_QUEUE_SIZE  ���size, ����̫��ʱ��ռ�ô������ڴ棬��Ҫע���ֵ������
	int		rindex = 0;                         // ��������������ʱ��ȡ��֡���в��ţ����ź��֡��Ϊ��һ֡
	int		windex = 0;                         // д����
	int		size = 0;                           // ��ǰ��֡��
	int		max_size = 0;                       // �ɴ洢���֡��
	int		keep_last = 0;                      // = 1˵��Ҫ�ڶ������汣�����һ֡�����ݲ��ͷţ�ֻ�����ٶ��е�ʱ��Ž��������ͷ�
	int		rindex_shown = 0;                   // ��ʼ��Ϊ0�����keep_last=1ʹ��
	std::mutex*	mutex_t = nullptr;                     // ������
	std::condition_variable	*cond = nullptr;                      // ��������
	PacketQueue	*pktq = nullptr;                      // ���ݰ��������
} FrameQueue;

// ���ｲ��ϵͳʱ�� ��ͨ��av_gettime_relative()��ȡ����ʱ�ӣ���λΪ΢��
typedef struct Clock {
	double	pts;            // ʱ�ӻ���, ��ǰ֡(������)��ʾʱ��������ź󣬵�ǰ֡�����һ֡
	// ��ǰpts�뵱ǰϵͳʱ�ӵĲ�ֵ, audio��video���ڸ�ֵ�Ƕ�����
	double	pts_drift;      // clock base minus time at which we updated the clock
	// ��ǰʱ��(����Ƶʱ��)���һ�θ���ʱ�䣬Ҳ�ɳƵ�ǰʱ��ʱ��
	double	last_updated;   // ���һ�θ��µ�ϵͳʱ��
	double	speed;          // ʱ���ٶȿ��ƣ����ڿ��Ʋ����ٶ�
	// �������У���ν�������о���һ�������Ĳ��Ŷ�����һ��seek����������һ���µĲ�������
	int	serial;             // clock is based on a packet with this serial
	int	paused;             // = 1 ˵������ͣ״̬
	// ָ��packet_serial
	int *queue_serial;      /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;

//typedef struct VideoState {
//	AVClock avclock;
//}VideoState;

// ����Ƶ�������
class AVPacketQueue {

public:
	AVPacketQueue();
	~AVPacketQueue();

	//////////////////////packet///////////////////////

	// ���г�ʼ��
	int  packet_vidio_queue_init();
	int  packet_audio_queue_init();

	// ��������
	void packet_video_queue_destroy();
	void packet_audio_queue_destroy();

	// ���������˳�
	void packet_video_queue_about() { packet_queue_about(&m_video_packet_queue); }
	void packet_audio_queue_about() { packet_queue_about(&m_audio_packet_queue); }

	// ��ն���
	void packet_video_queue_flash() { packet_queue_flush(&m_video_packet_queue); }
	void packet_audio_queue_flash() { packet_queue_flush(&m_audio_packet_queue); }

	// ���ö���
	void packet_video_queue_start() { packet_queue_start(&m_video_packet_queue); }
	void packet_audio_queue_start() { packet_queue_start(&m_audio_packet_queue); }

	// ����flash_pktͬʱ��ս�����
	void video_queue_put_flash() { packet_video_queue_put(&m_flush_pkt); }
	void audio_queue_put_flash() { packet_audio_queue_put(&m_flush_pkt); }

	// ����հ�
	void video_queue_put_nullpacket(int stream_index);
	void audio_queue_put_nullpacket(int stream_index);

	// �����з���һ֡
	void packet_video_queue_put(AVPacket* avpacket);
	void packet_audio_queue_put(AVPacket* avpacket);

	// �Ӷ��л�ȡAVPacket
	int video_queue_get(AVPacket* pkt, int& pkt_serial);
	int audio_queue_get(AVPacket* pkt, int& pkt_serial);

	// �ж��Ƿ���Ҫˢ�¶���
	bool isNeedFlashbuffers(AVPacket* pkt) { return pkt->data == m_flush_pkt.data; }

	// ��ȡ����ָ��
	PacketQueue* get_video_packet_point() { return &m_video_packet_queue; }
	PacketQueue* get_audio_packet_point() { return &m_audio_packet_queue; }

	// ��ȡ����size
	int get_video_packet_size() { return m_video_packet_queue.size; }
	int get_audio_packet_size() { return m_audio_packet_queue.size; }

	// ��ȡ������Ƶ����ָ��
	FrameQueue* get_frame_video_queue() { return &m_video_frame_queue; }
	FrameQueue* get_frame_audio_queue() { return &m_audio_frame_queue; }

	/////////////////////frame/////////////////////////

	int frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last);

	// ��������frame�ŵ�����
	int frame_video_frame_put(AVFrame* src_frame, double pts, double duration, int64_t pos, int serial);
	int frame_audio_frame_put(AVFrame* src_frame, double pts, double duration, int64_t pos, int serial);

	// ���������˳�
	void frame_video_queue_signal() { frame_queue_signal(&m_video_frame_queue); }
	void frame_audio_queue_signal() { frame_queue_signal(&m_audio_frame_queue); }

	void frame_queue_video_destory() { frame_queue_destory(&m_video_frame_queue); }
	void frame_queue_audio_destory() { frame_queue_destory(&m_audio_frame_queue); }

	// ��frame��ȡһ֡
	Frame* video_frame_get() { return frame_queue_peek_readable(&m_video_frame_queue); }
	Frame* audio_frame_get() { return frame_queue_peek_readable(&m_audio_frame_queue); }

	// �ͷŵ�ǰframe�������¶�����rindex
	void video_frame_queue_next() { frame_queue_next(&m_video_frame_queue); }
	void audio_frame_queue_next() { frame_queue_next(&m_audio_frame_queue); }

	// ��ȡ�������Ƿ���֡��ʾ
	int video_frame_queue_nb_remaining() { return frame_queue_nb_remaining(&m_video_frame_queue); }
	int audio_frame_queue_nb_remaining() { return frame_queue_nb_remaining(&m_audio_frame_queue); }

	// ��ȡ��һ֡
	Frame* video_frame_queue_peek_last() { return frame_queue_peek_last(&m_video_frame_queue); }
	// ��ȡ���е�ǰFrame
	Frame* video_frame_queue_peek() { return frame_queue_peek(&m_video_frame_queue); }
	// ��ȡ��ǰFrame����һFrame, ��ʱҪȷ��queue����������2��Frame
	Frame* video_frame_queue_peek_next() { return frame_queue_peek_next(&m_video_frame_queue); }

private:
	int  packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial);
	int  packet_queue_put(PacketQueue *q, AVPacket *pkt);
	void packet_queue_flush(PacketQueue *q);
	void packet_queue_start(PacketQueue *q);
	int  packet_queue_put_private(PacketQueue *q, AVPacket *pkt);
	// ��ȡ��дָ��
	Frame *frame_queue_peek_writable(FrameQueue *f);
	// ����дָ��
	void frame_queue_push(FrameQueue *f);
	// ��ȡָ��ɶ�֡
	Frame *frame_queue_peek_readable(FrameQueue *f);
	// �ͷ��ڴ�
	void frame_queue_unref_item(Frame *vp);
	/* �ͷŵ�ǰframe�������¶�����rindex��
    * ��keep_lastΪ1, rindex_showΪ0ʱ��ȥ����rindex,Ҳ���ͷŵ�ǰframe */
	void frame_queue_next(FrameQueue *f);
	/* return the number of undisplayed frames in the queue */
	int frame_queue_nb_remaining(FrameQueue *f);
	Frame *frame_queue_peek_last(FrameQueue *f);
	/* ��ȡ���е�ǰFrame, �ڵ��øú���ǰ�ȵ���frame_queue_nb_remainingȷ����frame�ɶ� */
	Frame *frame_queue_peek(FrameQueue *f);
	/* ��ȡ��ǰFrame����һFrame, ��ʱҪȷ��queue����������2��Frame */
    // ������ʲôʱ����ã��������϶����� NULL
	Frame *frame_queue_peek_next(FrameQueue *f);
	// ���������˳�
	void packet_queue_about(PacketQueue *q);
	void frame_queue_signal(FrameQueue *f);
	// farme destory
	void frame_queue_destory(FrameQueue *f);

private:
	AVPacket m_flush_pkt; // ����ˢ�¶���

	PacketQueue m_video_packet_queue; // δ������Ƶ����
	PacketQueue m_audio_packet_queue; // δ������Ƶ����

	FrameQueue m_video_frame_queue;   // ��������Ƶ����
	FrameQueue m_audio_frame_queue;   // ��������Ƶ����
};

// ʱ����
class AVClock {
public:
	AVClock();
	~AVClock();

	//void adjust_clock();

	void set_clock_at(Clock *c, double pts, int serial, double time);
	void set_clock(Clock *c, double pts, int serial);
	// ��ʼ��ʱ��
	void init_clock(Clock *c, int* queue_serial);

	/**
     * ��ȡ����ʵ������:���һ֡��pts ���� �Ӵ������һ֡��ʼ�����ڵ�ʱ��,����ο�set_clock_at ��get_clock�Ĵ���
     * c->pts_drift=���һ֡��pts-�Ӵ������һ֡ʱ��
     * clock=c->pts_drift+���ڵ�ʱ��
     * get_clock(&is->vidclk) ==is->vidclk.pts, av_gettime_relative() / 1000000.0 -is->vidclk.last_updated  +is->vidclk.pts
     */
	double get_clock(Clock *c)
	{
		if (*c->queue_serial != c->serial)
			return NAN; // ����ͬһ���������У�ʱ������Ч
		if (c->paused) {
			return c->pts;  // ��ͣ��ʱ�򷵻ص���pts
		}
		else {
			double time = av_gettime_relative() / 1000000.0;
			return c->pts_drift + time - (time - c->last_updated) * (1.0 - c->speed);
		}
	}

	/* get the current master clock value */
	double get_master_clock()
	{
		double val;

		switch (get_master_sync_type()) {
		case AV_SYNC_VIDEO_MASTER:
			val = get_clock(&vidclk);
			break;
		case AV_SYNC_AUDIO_MASTER:
			val = get_clock(&audclk);
			break;
		default:
			val = get_clock(&extclk);
			break;
		}
		return val;
	}

	int get_master_sync_type() {
		//if (is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
		//	if (is->video_st)
		//		return AV_SYNC_VIDEO_MASTER;
		//	else
		//		return AV_SYNC_AUDIO_MASTER;	 /* ���û����Ƶ�ɷ���ʹ�� audio master */
		//}
		//else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
		//	if (is->audio_st)
		//		return AV_SYNC_AUDIO_MASTER;
		//	else
		//		return AV_SYNC_EXTERNAL_CLOCK;	 /* û����Ƶ��ʱ���Ǿ����ⲿʱ�� */
		//}
		//else {
		//	return AV_SYNC_EXTERNAL_CLOCK;
		//}
		return av_sync_type;
	}

	void sync_clock_to_slave(Clock *c, Clock *slave)
	{
		double clock = get_clock(c);
		double slave_clock = get_clock(slave);
		if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD))
			set_clock(c, slave_clock, slave->serial);
	}

	void update_video_pts(double pts, int64_t pos, float playback_rate, int serial) {
		/* update current video pts */
		set_clock(&vidclk, pts / playback_rate, serial);
		sync_clock_to_slave(&extclk, &vidclk);
	}

	Clock	audclk;             // ��Ƶʱ��
	Clock	vidclk;             // ��Ƶʱ��
	Clock	extclk;             // �ⲿʱ��

	int av_sync_type = AV_SYNC_AUDIO_MASTER;           // ����Ƶͬ������, Ĭ��audio master

	double	audio_clock;            // ��ǰ��Ƶ֡��PTS+��ǰ֡Duration
	int     audio_clock_serial;     // �������У�seek�ɸı��ֵ

	double max_frame_duration;      // ��Ƶһ֡�����. above this, we consider the jump a timestamp discontinuity
};

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
#define VIDEO_PICTURE_QUEUE_SIZE	3       // 图像帧缓存数量
#define SUBPICTURE_QUEUE_SIZE		16      // 字幕帧缓存数量
#define SAMPLE_QUEUE_SIZE           9       // 采样帧缓存数量
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
 *音视频同步方式，缺省以音频为基准
 */
enum {
	AV_SYNC_AUDIO_MASTER,                   // 以音频为基准
	AV_SYNC_VIDEO_MASTER,                   // 以视频为基准
	AV_SYNC_EXTERNAL_CLOCK,                 // 以外部时钟为基准，synchronize to an external clock */
};

// AVPacket队列
typedef struct MyAVPacketList {
	AVPacket		pkt;    //解封装后的数据,av_read_frame获取的AVPacket做浅拷贝
	struct MyAVPacketList	*next;  //下一个节点
	int			serial = 0;     //播放序列
} MyAVPacketList;

typedef struct PacketQueue {
	MyAVPacketList	*first_pkt = nullptr, *last_pkt = nullptr;  // 队首，队尾指针
	int		nb_packets = 0;      // 包数量，也就是队列元素数量
	int		size = 0;            // 队列所有元素的数据大小总和
	int64_t		duration = 0;    // 队列所有元素的数据播放持续时间
	int		abort_request = 0;   // 用户退出请求标志
	int		serial = 0;          // 播放序列号，和MyAVPacketList的serial作用相同，但改变的时序稍微有点不同
	std::mutex* mutex_t = nullptr;       // 用于维持PacketQueue的多线程安全(可以按pthread_mutex_t理解）
	std::condition_variable* cond_t = nullptr;      // 用于读、写线程相互通知(可以按pthread_cond_t理解)
} PacketQueue;

typedef struct AudioParams {
	int			freq;                   // 采样率
	int			channels;               // 通道数
	int64_t		channel_layout;         // 通道布局，比如2.1声道，5.1声道等
	enum AVSampleFormat	fmt;            // 音频采样格式，比如AV_SAMPLE_FMT_S16表示为有符号16bit深度，交错排列模式。
	int			frame_size;             // 一个采样单元占用的字节数（比如2通道时，则左右通道各采样一次合成一个采样单元）
	int			bytes_per_sec;          // 一秒时间的字节数，比如采样率48Khz，2 channel，16bit，则一秒48000*2*16/8=192000
} AudioParams;

// ffmpeg 解码器
typedef struct FFDecoder {
	AVStream *video_st = nullptr;             // 视频流

	//
	AVPacket pkt;
	PacketQueue	*queue = nullptr;                  // 数据包队列
	AVFormatContext* avformat_context = nullptr;   // iformat的上下文
	AVCodecContext* codec_context = nullptr;       // 解码器上下文
	AVCodec* avcodec = nullptr;                    // 解码器
	int		pkt_serial = 0;         // 包序列
	int		finished = 0;           // =0，解码器处于工作状态；=非0，解码器处于空闲状态
	int		packet_pending = 0;     // =0，解码器处于异常状态，需要考虑重置解码器；=1，解码器处于正常状态
	std::condition_variable	*empty_queue_cond = nullptr;  // 检查到packet队列空时发送 signal缓存read_thread读取数据
	int64_t		start_pts = 0;          // 初始化时是stream的start time
	AVRational	start_pts_tb;       // 初始化时是stream的time_base
	int64_t		next_pts = 0;           // 记录最近一次解码后的frame的pts，当解出来的部分帧没有有效的pts时则使用next_pts进行推算
	AVRational	next_pts_tb;        // next_pts的单位
}FFDecoder;

// 用于缓存解码后的数据
typedef struct Frame {
	AVFrame		*frame;         // 指向数据帧
	AVSubtitle	sub;            // 用于字幕
	int		serial;             // 帧序列，在seek的操作时serial会变化
	double		pts;            // 时间戳，单位为秒
	double		duration;       // 该帧持续时间，单位为秒
	int64_t		pos;            // 该帧在输入文件中的字节位置
	int		width;              // 图像宽度
	int		height;             // 图像高读
	int		format;             // 对于图像为(enum AVPixelFormat)，
	// 对于声音则为(enum AVSampleFormat)
	AVRational	sar;            // 图像的宽高比（16:9，4:3...），如果未知或未指定则为0/1
	int		uploaded;           // 用来记录该帧是否已经显示过？
	int		flip_v;             // =1则垂直翻转， = 0则正常播放
} Frame;

/* 这是一个循环队列，windex是指其中的首元素，rindex是指其中的尾部元素. */
typedef struct FrameQueue {
	Frame	queue[FRAME_QUEUE_SIZE];        // FRAME_QUEUE_SIZE  最大size, 数字太大时会占用大量的内存，需要注意该值的设置
	int		rindex = 0;                         // 读索引。待播放时读取此帧进行播放，播放后此帧成为上一帧
	int		windex = 0;                         // 写索引
	int		size = 0;                           // 当前总帧数
	int		max_size = 0;                       // 可存储最大帧数
	int		keep_last = 0;                      // = 1说明要在队列里面保持最后一帧的数据不释放，只在销毁队列的时候才将其真正释放
	int		rindex_shown = 0;                   // 初始化为0，配合keep_last=1使用
	std::mutex*	mutex_t = nullptr;                     // 互斥量
	std::condition_variable	*cond = nullptr;                      // 条件变量
	PacketQueue	*pktq = nullptr;                      // 数据包缓冲队列
} FrameQueue;

// 这里讲的系统时钟 是通过av_gettime_relative()获取到的时钟，单位为微妙
typedef struct Clock {
	double	pts;            // 时钟基础, 当前帧(待播放)显示时间戳，播放后，当前帧变成上一帧
	// 当前pts与当前系统时钟的差值, audio、video对于该值是独立的
	double	pts_drift;      // clock base minus time at which we updated the clock
	// 当前时钟(如视频时钟)最后一次更新时间，也可称当前时钟时间
	double	last_updated;   // 最后一次更新的系统时钟
	double	speed;          // 时钟速度控制，用于控制播放速度
	// 播放序列，所谓播放序列就是一段连续的播放动作，一个seek操作会启动一段新的播放序列
	int	serial;             // clock is based on a packet with this serial
	int	paused;             // = 1 说明是暂停状态
	// 指向packet_serial
	int *queue_serial;      /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;

//typedef struct VideoState {
//	AVClock avclock;
//}VideoState;

// 音视频解码队列
class AVPacketQueue {

public:
	AVPacketQueue();
	~AVPacketQueue();

	//////////////////////packet///////////////////////

	// 队列初始化
	int  packet_vidio_queue_init();
	int  packet_audio_queue_init();

	// 队列销毁
	void packet_video_queue_destroy();
	void packet_audio_queue_destroy();

	// 队列请求退出
	void packet_video_queue_about() { packet_queue_about(&m_video_packet_queue); }
	void packet_audio_queue_about() { packet_queue_about(&m_audio_packet_queue); }

	// 清空队列
	void packet_video_queue_flash() { packet_queue_flush(&m_video_packet_queue); }
	void packet_audio_queue_flash() { packet_queue_flush(&m_audio_packet_queue); }

	// 启用队列
	void packet_video_queue_start() { packet_queue_start(&m_video_packet_queue); }
	void packet_audio_queue_start() { packet_queue_start(&m_audio_packet_queue); }

	// 放入flash_pkt同时清空解码器
	void video_queue_put_flash() { packet_video_queue_put(&m_flush_pkt); }
	void audio_queue_put_flash() { packet_audio_queue_put(&m_flush_pkt); }

	// 放入空包
	void video_queue_put_nullpacket(int stream_index);
	void audio_queue_put_nullpacket(int stream_index);

	// 往队列放入一帧
	void packet_video_queue_put(AVPacket* avpacket);
	void packet_audio_queue_put(AVPacket* avpacket);

	// 从队列获取AVPacket
	int video_queue_get(AVPacket* pkt, int& pkt_serial);
	int audio_queue_get(AVPacket* pkt, int& pkt_serial);

	// 判断是否需要刷新队列
	bool isNeedFlashbuffers(AVPacket* pkt) { return pkt->data == m_flush_pkt.data; }

	// 获取队列指针
	PacketQueue* get_video_packet_point() { return &m_video_packet_queue; }
	PacketQueue* get_audio_packet_point() { return &m_audio_packet_queue; }

	// 获取队列size
	int get_video_packet_size() { return m_video_packet_queue.size; }
	int get_audio_packet_size() { return m_audio_packet_queue.size; }

	// 获取解码视频队列指针
	FrameQueue* get_frame_video_queue() { return &m_video_frame_queue; }
	FrameQueue* get_frame_audio_queue() { return &m_audio_frame_queue; }

	/////////////////////frame/////////////////////////

	int frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last);

	// 将解码后的frame放到队列
	int frame_video_frame_put(AVFrame* src_frame, double pts, double duration, int64_t pos, int serial);
	int frame_audio_frame_put(AVFrame* src_frame, double pts, double duration, int64_t pos, int serial);

	// 队列请求退出
	void frame_video_queue_signal() { frame_queue_signal(&m_video_frame_queue); }
	void frame_audio_queue_signal() { frame_queue_signal(&m_audio_frame_queue); }

	void frame_queue_video_destory() { frame_queue_destory(&m_video_frame_queue); }
	void frame_queue_audio_destory() { frame_queue_destory(&m_audio_frame_queue); }

	// 从frame获取一帧
	Frame* video_frame_get() { return frame_queue_peek_readable(&m_video_frame_queue); }
	Frame* audio_frame_get() { return frame_queue_peek_readable(&m_audio_frame_queue); }

	// 释放当前frame，并更新读索引rindex
	void video_frame_queue_next() { frame_queue_next(&m_video_frame_queue); }
	void audio_frame_queue_next() { frame_queue_next(&m_audio_frame_queue); }

	// 获取队列中是否有帧显示
	int video_frame_queue_nb_remaining() { return frame_queue_nb_remaining(&m_video_frame_queue); }
	int audio_frame_queue_nb_remaining() { return frame_queue_nb_remaining(&m_audio_frame_queue); }

	// 获取上一帧
	Frame* video_frame_queue_peek_last() { return frame_queue_peek_last(&m_video_frame_queue); }
	// 获取队列当前Frame
	Frame* video_frame_queue_peek() { return frame_queue_peek(&m_video_frame_queue); }
	// 获取当前Frame的下一Frame, 此时要确保queue里面至少有2个Frame
	Frame* video_frame_queue_peek_next() { return frame_queue_peek_next(&m_video_frame_queue); }

private:
	int  packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial);
	int  packet_queue_put(PacketQueue *q, AVPacket *pkt);
	void packet_queue_flush(PacketQueue *q);
	void packet_queue_start(PacketQueue *q);
	int  packet_queue_put_private(PacketQueue *q, AVPacket *pkt);
	// 获取可写指针
	Frame *frame_queue_peek_writable(FrameQueue *f);
	// 更新写指针
	void frame_queue_push(FrameQueue *f);
	// 获取指向可读帧
	Frame *frame_queue_peek_readable(FrameQueue *f);
	// 释放内存
	void frame_queue_unref_item(Frame *vp);
	/* 释放当前frame，并更新读索引rindex，
    * 当keep_last为1, rindex_show为0时不去更新rindex,也不释放当前frame */
	void frame_queue_next(FrameQueue *f);
	/* return the number of undisplayed frames in the queue */
	int frame_queue_nb_remaining(FrameQueue *f);
	Frame *frame_queue_peek_last(FrameQueue *f);
	/* 获取队列当前Frame, 在调用该函数前先调用frame_queue_nb_remaining确保有frame可读 */
	Frame *frame_queue_peek(FrameQueue *f);
	/* 获取当前Frame的下一Frame, 此时要确保queue里面至少有2个Frame */
    // 不管你什么时候调用，返回来肯定不是 NULL
	Frame *frame_queue_peek_next(FrameQueue *f);
	// 队列请求退出
	void packet_queue_about(PacketQueue *q);
	void frame_queue_signal(FrameQueue *f);
	// farme destory
	void frame_queue_destory(FrameQueue *f);

private:
	AVPacket m_flush_pkt; // 用于刷新队列

	PacketQueue m_video_packet_queue; // 未解码视频队列
	PacketQueue m_audio_packet_queue; // 未解码音频队列

	FrameQueue m_video_frame_queue;   // 解码后的视频队列
	FrameQueue m_audio_frame_queue;   // 解码后的音频队列
};

// 时钟类
class AVClock {
public:
	AVClock();
	~AVClock();

	//void adjust_clock();

	void set_clock_at(Clock *c, double pts, int serial, double time);
	void set_clock(Clock *c, double pts, int serial);
	// 初始化时钟
	void init_clock(Clock *c, int* queue_serial);

	/**
     * 获取到的实际上是:最后一帧的pts 加上 从处理最后一帧开始到现在的时间,具体参考set_clock_at 和get_clock的代码
     * c->pts_drift=最后一帧的pts-从处理最后一帧时间
     * clock=c->pts_drift+现在的时候
     * get_clock(&is->vidclk) ==is->vidclk.pts, av_gettime_relative() / 1000000.0 -is->vidclk.last_updated  +is->vidclk.pts
     */
	double get_clock(Clock *c)
	{
		if (*c->queue_serial != c->serial)
			return NAN; // 不是同一个播放序列，时钟是无效
		if (c->paused) {
			return c->pts;  // 暂停的时候返回的是pts
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
		//		return AV_SYNC_AUDIO_MASTER;	 /* 如果没有视频成分则使用 audio master */
		//}
		//else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
		//	if (is->audio_st)
		//		return AV_SYNC_AUDIO_MASTER;
		//	else
		//		return AV_SYNC_EXTERNAL_CLOCK;	 /* 没有音频的时候那就用外部时钟 */
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

	Clock	audclk;             // 音频时钟
	Clock	vidclk;             // 视频时钟
	Clock	extclk;             // 外部时钟

	int av_sync_type = AV_SYNC_AUDIO_MASTER;           // 音视频同步类型, 默认audio master

	double	audio_clock;            // 当前音频帧的PTS+当前帧Duration
	int     audio_clock_serial;     // 播放序列，seek可改变此值

	double max_frame_duration;      // 视频一帧最大间隔. above this, we consider the jump a timestamp discontinuity
};

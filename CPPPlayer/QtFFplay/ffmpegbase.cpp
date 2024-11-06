#include "ffmpegbase.h"

AVPacketQueue::AVPacketQueue() {
	av_init_packet(&m_flush_pkt);				// 初始化flush_packet
	m_flush_pkt.data = (uint8_t *)&m_flush_pkt; // 初始化为数据指向自己本身
}

AVPacketQueue::~AVPacketQueue() {
}

int  AVPacketQueue::packet_vidio_queue_init() {
	m_video_packet_queue.mutex_t = new std::mutex;
	m_video_packet_queue.cond_t = new std::condition_variable;
	m_video_packet_queue.abort_request = 1;
	return 0;
}

int AVPacketQueue::packet_audio_queue_init() {
	m_audio_packet_queue.mutex_t = new std::mutex;
	m_audio_packet_queue.cond_t = new std::condition_variable;
	m_audio_packet_queue.abort_request = 1;
	return 0;
}

void AVPacketQueue::packet_video_queue_destroy() {
	packet_queue_flush(&m_video_packet_queue);

	if (m_video_packet_queue.mutex_t != nullptr) {
		delete m_video_packet_queue.mutex_t;
		m_video_packet_queue.mutex_t = nullptr;
	}

	if (m_video_packet_queue.cond_t != nullptr) {
		delete m_video_packet_queue.cond_t;
		m_video_packet_queue.cond_t = nullptr;
	}
}

void AVPacketQueue::packet_audio_queue_destroy() {
	packet_queue_flush(&m_audio_packet_queue);

	if (m_audio_packet_queue.mutex_t != nullptr) {
		delete m_audio_packet_queue.mutex_t;
		m_audio_packet_queue.mutex_t = nullptr;
	}

	if (m_audio_packet_queue.cond_t != nullptr) {
		delete m_audio_packet_queue.cond_t;
		m_audio_packet_queue.cond_t = nullptr;
	}
}

void AVPacketQueue::video_queue_put_nullpacket(int stream_index)
{
	AVPacket pkt1, *pkt = &pkt1;
	av_init_packet(pkt);
	pkt->data = NULL;
	pkt->size = 0;
	pkt->stream_index = stream_index;
	return packet_video_queue_put(pkt);
}

void AVPacketQueue::audio_queue_put_nullpacket(int stream_index)
{
	AVPacket pkt1, *pkt = &pkt1;
	av_init_packet(pkt);
	pkt->data = NULL;
	pkt->size = 0;
	pkt->stream_index = stream_index;
	return packet_audio_queue_put(pkt);
}

void AVPacketQueue::packet_video_queue_put(AVPacket* avpacket)
{
	packet_queue_put(&m_video_packet_queue, avpacket);
}

void AVPacketQueue::packet_audio_queue_put(AVPacket* avpacket)
{
	packet_queue_put(&m_audio_packet_queue, avpacket);
}

int AVPacketQueue::video_queue_get(AVPacket* pkt, int& pkt_serial) {
	return packet_queue_get(&m_video_packet_queue, pkt, 1,&pkt_serial);
}

int AVPacketQueue::audio_queue_get(AVPacket* pkt, int& pkt_serial) {
	return packet_queue_get(&m_audio_packet_queue, pkt, 1, &pkt_serial);
}

int AVPacketQueue::packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial)
{
	MyAVPacketList *pkt1;
	int ret;

	std::unique_lock<std::mutex> lock(*q->mutex_t);   // 加锁

	for (;;) {
		if (q->abort_request) {
			ret = -1;
			break;
		}

		pkt1 = q->first_pkt;    //MyAVPacketList *pkt1; 从队头拿数据
		if (pkt1) {     //队列中有数据
			q->first_pkt = pkt1->next;  //队头移到第二个节点
			if (!q->first_pkt)
				q->last_pkt = NULL;
			q->nb_packets--;    //节点数减1
			q->size -= pkt1->pkt.size + sizeof(*pkt1);  //cache大小扣除一个节点
			q->duration -= pkt1->pkt.duration;  //总时长扣除一个节点
			//返回AVPacket，这里发生一次AVPacket结构体拷贝，AVPacket的data只拷贝了指针
			*pkt = pkt1->pkt;
			if (serial) //如果需要输出serial，把serial输出
				*serial = pkt1->serial;
			av_free(pkt1);      //释放节点内存,只是释放节点，而不是释放AVPacket
			ret = 1;
			break;
		}
		else if (!block) {    //队列中没有数据，且非阻塞调用
			ret = 0;
			break;
		}
		else {
			//队列中没有数据，且阻塞调用
			//这里没有break。for循环的另一个作用是在条件变量满足后重复上述代码取出节点
			q->cond_t->wait(lock);
		}
	}
	//q->mutex_t->unlock();  // 释放锁
	return ret;
}

int AVPacketQueue::packet_queue_put(PacketQueue *q, AVPacket *pkt) {
	int ret;

	std::unique_lock<std::mutex> lock(*q->mutex_t);
	ret = packet_queue_put_private(q, pkt);//主要实现
	//q->mutex_t->unlock();

	if (pkt != &m_flush_pkt && ret < 0)
		av_packet_unref(pkt);       //放入失败，释放AVPacket

	return ret;
}

// 清空队列
void AVPacketQueue::packet_queue_flush(PacketQueue *q)
{
	MyAVPacketList *pkt, *pkt1;
	if (q->last_pkt == NULL && q->first_pkt == NULL)
		return;

	std::unique_lock<std::mutex> lock(*q->mutex_t);
	for (pkt = q->first_pkt; pkt; pkt = pkt1) {
		pkt1 = pkt->next;
		av_packet_unref(&pkt->pkt);
		av_freep(&pkt);
	}
	q->last_pkt = NULL;
	q->first_pkt = NULL;
	q->nb_packets = 0;
	q->size = 0;
	q->duration = 0;
	//q->mutex_t->unlock();
}

// 启用队列
void AVPacketQueue::packet_queue_start(PacketQueue *q)
{
	std::unique_lock<std::mutex> lock(*q->mutex_t);
	q->abort_request = 0;
	packet_queue_put_private(q, &m_flush_pkt); //这里放入了一个flush_pkt
	//q->mutex_t->unlock();
}

int AVPacketQueue::packet_queue_put_private(PacketQueue *q, AVPacket *pkt)
{
	MyAVPacketList *pkt1;

	if (q->abort_request)   //如果已中止，则放入失败
		return -1;

	pkt1 = (MyAVPacketList*)av_malloc(sizeof(MyAVPacketList));   //分配节点内存
	if (!pkt1)  //内存不足，则放入失败
		return -1;
	// 没有做引用计数，那这里也说明av_read_frame不会释放替用户释放buffer。
	pkt1->pkt = *pkt; //拷贝AVPacket(浅拷贝，AVPacket.data等内存并没有拷贝)
	pkt1->next = NULL;
	if (pkt == &m_flush_pkt)//如果放入的是flush_pkt，需要增加队列的播放序列号，以区分不连续的两段数据
	{
		q->serial++;
		printf("q->serial = %d\n", q->serial);
	}
	pkt1->serial = q->serial;   //用队列序列号标记节点
	/* 队列操作：如果last_pkt为空，说明队列是空的，新增节点为队头；
	 * 否则，队列有数据，则让原队尾的next为新增节点。 最后将队尾指向新增节点
	 */
	if (!q->last_pkt)
		q->first_pkt = pkt1;
	else
		q->last_pkt->next = pkt1;
	q->last_pkt = pkt1;

	//队列属性操作：增加节点数、cache大小、cache总时长, 用来控制队列的大小
	q->nb_packets++;
	q->size += pkt1->pkt.size + sizeof(*pkt1);
	q->duration += pkt1->pkt.duration;

	/* XXX: should duplicate packet data in DV case */
	//发出信号，表明当前队列中有数据了，通知等待中的读线程可以取数据了
	q->cond_t->notify_one();
	return 0;
}

Frame* AVPacketQueue::frame_queue_peek_writable(FrameQueue *f)
{
	/* wait until we have space to put a new frame */
	std::unique_lock<std::mutex> lock(*f->mutex_t);

	while (f->size >= f->max_size &&
		!f->pktq->abort_request) {	/* 检查是否需要退出 */
		f->cond->wait(lock);
	}
	if (f->pktq->abort_request)			 /* 检查是不是要退出 */
		return NULL;

	return &f->queue[f->windex];
}

void AVPacketQueue::frame_queue_push(FrameQueue *f)
{
	if (++f->windex == f->max_size)
		f->windex = 0;
	std::unique_lock<std::mutex> lock(*f->mutex_t);

	f->size++;
	f->cond->notify_one();    // 当_readable在等待时则可以唤醒
}

Frame* AVPacketQueue::frame_queue_peek_readable(FrameQueue *f)
{
	/* wait until we have a readable a new frame */
	std::unique_lock<std::mutex> lock(*f->mutex_t);
	while (f->size - f->rindex_shown <= 0 &&
		!f->pktq->abort_request) {
		f->cond->wait(lock);
	}

	if (f->pktq->abort_request)
		return NULL;

	return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

void AVPacketQueue::frame_queue_unref_item(Frame *vp)
{
	av_frame_unref(vp->frame);	/* 释放数据 */
	avsubtitle_free(&vp->sub);
}

/* 释放当前frame，并更新读索引rindex，
* 当keep_last为1, rindex_show为0时不去更新rindex,也不释放当前frame */
void AVPacketQueue::frame_queue_next(FrameQueue *f)
{
	if (f->keep_last && !f->rindex_shown) {
		f->rindex_shown = 1; // 第一次进来没有更新，对应的frame就没有释放
		return;
	}
	frame_queue_unref_item(&f->queue[f->rindex]);
	if (++f->rindex == f->max_size)
		f->rindex = 0;
	std::unique_lock<std::mutex> lock(*f->mutex_t);
	f->size--;
	f->cond->notify_one();
}

Frame* AVPacketQueue::frame_queue_peek_last(FrameQueue *f) {
	return &f->queue[f->rindex];    // 这时候才有意义
}

/* 获取队列当前Frame, 在调用该函数前先调用frame_queue_nb_remaining确保有frame可读 */
Frame* AVPacketQueue::frame_queue_peek(FrameQueue *f) {
	return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

/* 获取当前Frame的下一Frame, 此时要确保queue里面至少有2个Frame */
// 不管你什么时候调用，返回来肯定不是 NULL
Frame* AVPacketQueue::frame_queue_peek_next(FrameQueue *f) {
	return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

void AVPacketQueue::packet_queue_about(PacketQueue *q) {
	std::unique_lock<std::mutex> lock(*q->mutex_t);
	q->abort_request = 1;       // 请求退出
	q->cond_t->notify_one();    //释放一个条件信号
}

void AVPacketQueue::frame_queue_signal(FrameQueue *f)
{
	std::unique_lock<std::mutex> lock(*f->mutex_t);
	f->cond->notify_one();
}

void AVPacketQueue::frame_queue_destory(FrameQueue *f)
{
	if (f->pktq == nullptr)
		return;

	int i;
	for (i = 0; i < f->max_size; i++) {
		Frame *vp = &f->queue[i];
		// 释放对vp->frame中的数据缓冲区的引用，注意不是释放frame对象本身
		frame_queue_unref_item(vp);
		// 释放vp->frame对象
		av_frame_free(&vp->frame);
	}
	std::unique_lock<std::mutex> lock(*f->mutex_t);
	f->cond->notify_one();
}

int AVPacketQueue::frame_queue_nb_remaining(FrameQueue *f)
{
	return f->size - f->rindex_shown;	// 注意这里为什么要减去f->rindex_shown
}

int AVPacketQueue::frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last) {
	int i;
	memset(f, 0, sizeof(FrameQueue));
	if (!(f->mutex_t = new std::mutex)) {
		//av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n");
		return AVERROR(ENOMEM);
	}
	if (!(f->cond = new std::condition_variable)) {
		//av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
		return AVERROR(ENOMEM);
	}
	f->pktq = pktq;
	f->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
	f->keep_last = !!keep_last;
	for (i = 0; i < f->max_size; i++)
		if (!(f->queue[i].frame = av_frame_alloc())) // 分配AVFrame结构体
			return AVERROR(ENOMEM);
	return 0;
}

int AVPacketQueue::frame_video_frame_put(AVFrame* src_frame, double pts, double duration, int64_t pos, int serial) {
	Frame* vp;

	if (!(vp = frame_queue_peek_writable(&m_video_frame_queue)))
		return -1;

	// 执行到这步说已经获取到了可写入的Frame
	vp->sar = src_frame->sample_aspect_ratio;
	vp->uploaded = 0;

	vp->width = src_frame->width;
	vp->height = src_frame->height;
	vp->format = src_frame->format;

	vp->pts = pts;
	vp->duration = duration;
	vp->pos = pos;
	vp->serial = serial;

	//set_default_window_size(vp->width, vp->height, vp->sar);

	av_frame_move_ref(vp->frame, src_frame); // 将src中所有数据转移到dst中，并复位src。
	frame_queue_push(&m_video_frame_queue);   // 更新写索引位置
	return 0;
}

int AVPacketQueue::frame_audio_frame_put(AVFrame* src_frame, double pts, double duration, int64_t pos, int serial) {
	Frame *af;

	// 获取可写frame
	if (!(af = frame_queue_peek_writable(&m_audio_frame_queue)))
		return -1;

	// 3. 设置Frame并放入FrameQueue
	af->pts = pts;
	af->pos = pos;
	af->serial = serial;
	af->duration = duration;

	av_frame_move_ref(af->frame, src_frame);
	frame_queue_push(&m_audio_frame_queue);
}

AVClock::AVClock() {

}

AVClock::~AVClock() {

}

void AVClock::set_clock_at(Clock *c, double pts, int serial, double time)
{
	c->pts = pts;                      /* 当前帧的pts */
	c->last_updated = time;            /* 最后更新的时间，实际上是当前的一个系统时间 */
	c->pts_drift = c->pts - time;      /* 当前帧pts和系统时间的差值，正常播放情况下两者的差值应该是比较固定的，因为两者都是以时间为基准进行线性增长 */
	c->serial = serial;
}
void AVClock::set_clock(Clock *c, double pts, int serial)
{
	double time = av_gettime_relative() / 1000000.0;
	set_clock_at(c, pts, serial, time);
}

void AVClock::init_clock(Clock *c, int* queue_serial) {
	c->speed = 1.0;
	c->paused = 0;
	c->queue_serial = queue_serial;
	set_clock(c, NAN, -1);
}
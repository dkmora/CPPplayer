#include "ffmpegbase.h"

AVPacketQueue::AVPacketQueue() {
	av_init_packet(&m_flush_pkt);				// ��ʼ��flush_packet
	m_flush_pkt.data = (uint8_t *)&m_flush_pkt; // ��ʼ��Ϊ����ָ���Լ�����
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

	std::unique_lock<std::mutex> lock(*q->mutex_t);   // ����

	for (;;) {
		if (q->abort_request) {
			ret = -1;
			break;
		}

		pkt1 = q->first_pkt;    //MyAVPacketList *pkt1; �Ӷ�ͷ������
		if (pkt1) {     //������������
			q->first_pkt = pkt1->next;  //��ͷ�Ƶ��ڶ����ڵ�
			if (!q->first_pkt)
				q->last_pkt = NULL;
			q->nb_packets--;    //�ڵ�����1
			q->size -= pkt1->pkt.size + sizeof(*pkt1);  //cache��С�۳�һ���ڵ�
			q->duration -= pkt1->pkt.duration;  //��ʱ���۳�һ���ڵ�
			//����AVPacket�����﷢��һ��AVPacket�ṹ�忽����AVPacket��dataֻ������ָ��
			*pkt = pkt1->pkt;
			if (serial) //�����Ҫ���serial����serial���
				*serial = pkt1->serial;
			av_free(pkt1);      //�ͷŽڵ��ڴ�,ֻ���ͷŽڵ㣬�������ͷ�AVPacket
			ret = 1;
			break;
		}
		else if (!block) {    //������û�����ݣ��ҷ���������
			ret = 0;
			break;
		}
		else {
			//������û�����ݣ�����������
			//����û��break��forѭ������һ������������������������ظ���������ȡ���ڵ�
			q->cond_t->wait(lock);
		}
	}
	//q->mutex_t->unlock();  // �ͷ���
	return ret;
}

int AVPacketQueue::packet_queue_put(PacketQueue *q, AVPacket *pkt) {
	int ret;

	std::unique_lock<std::mutex> lock(*q->mutex_t);
	ret = packet_queue_put_private(q, pkt);//��Ҫʵ��
	//q->mutex_t->unlock();

	if (pkt != &m_flush_pkt && ret < 0)
		av_packet_unref(pkt);       //����ʧ�ܣ��ͷ�AVPacket

	return ret;
}

// ��ն���
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

// ���ö���
void AVPacketQueue::packet_queue_start(PacketQueue *q)
{
	std::unique_lock<std::mutex> lock(*q->mutex_t);
	q->abort_request = 0;
	packet_queue_put_private(q, &m_flush_pkt); //���������һ��flush_pkt
	//q->mutex_t->unlock();
}

int AVPacketQueue::packet_queue_put_private(PacketQueue *q, AVPacket *pkt)
{
	MyAVPacketList *pkt1;

	if (q->abort_request)   //�������ֹ�������ʧ��
		return -1;

	pkt1 = (MyAVPacketList*)av_malloc(sizeof(MyAVPacketList));   //����ڵ��ڴ�
	if (!pkt1)  //�ڴ治�㣬�����ʧ��
		return -1;
	// û�������ü�����������Ҳ˵��av_read_frame�����ͷ����û��ͷ�buffer��
	pkt1->pkt = *pkt; //����AVPacket(ǳ������AVPacket.data���ڴ沢û�п���)
	pkt1->next = NULL;
	if (pkt == &m_flush_pkt)//����������flush_pkt����Ҫ���Ӷ��еĲ������кţ������ֲ���������������
	{
		q->serial++;
		printf("q->serial = %d\n", q->serial);
	}
	pkt1->serial = q->serial;   //�ö������кű�ǽڵ�
	/* ���в��������last_pktΪ�գ�˵�������ǿյģ������ڵ�Ϊ��ͷ��
	 * ���򣬶��������ݣ�����ԭ��β��nextΪ�����ڵ㡣 ��󽫶�βָ�������ڵ�
	 */
	if (!q->last_pkt)
		q->first_pkt = pkt1;
	else
		q->last_pkt->next = pkt1;
	q->last_pkt = pkt1;

	//�������Բ��������ӽڵ�����cache��С��cache��ʱ��, �������ƶ��еĴ�С
	q->nb_packets++;
	q->size += pkt1->pkt.size + sizeof(*pkt1);
	q->duration += pkt1->pkt.duration;

	/* XXX: should duplicate packet data in DV case */
	//�����źţ�������ǰ�������������ˣ�֪ͨ�ȴ��еĶ��߳̿���ȡ������
	q->cond_t->notify_one();
	return 0;
}

Frame* AVPacketQueue::frame_queue_peek_writable(FrameQueue *f)
{
	/* wait until we have space to put a new frame */
	std::unique_lock<std::mutex> lock(*f->mutex_t);

	while (f->size >= f->max_size &&
		!f->pktq->abort_request) {	/* ����Ƿ���Ҫ�˳� */
		f->cond->wait(lock);
	}
	if (f->pktq->abort_request)			 /* ����ǲ���Ҫ�˳� */
		return NULL;

	return &f->queue[f->windex];
}

void AVPacketQueue::frame_queue_push(FrameQueue *f)
{
	if (++f->windex == f->max_size)
		f->windex = 0;
	std::unique_lock<std::mutex> lock(*f->mutex_t);

	f->size++;
	f->cond->notify_one();    // ��_readable�ڵȴ�ʱ����Ի���
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
	av_frame_unref(vp->frame);	/* �ͷ����� */
	avsubtitle_free(&vp->sub);
}

/* �ͷŵ�ǰframe�������¶�����rindex��
* ��keep_lastΪ1, rindex_showΪ0ʱ��ȥ����rindex,Ҳ���ͷŵ�ǰframe */
void AVPacketQueue::frame_queue_next(FrameQueue *f)
{
	if (f->keep_last && !f->rindex_shown) {
		f->rindex_shown = 1; // ��һ�ν���û�и��£���Ӧ��frame��û���ͷ�
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
	return &f->queue[f->rindex];    // ��ʱ���������
}

/* ��ȡ���е�ǰFrame, �ڵ��øú���ǰ�ȵ���frame_queue_nb_remainingȷ����frame�ɶ� */
Frame* AVPacketQueue::frame_queue_peek(FrameQueue *f) {
	return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

/* ��ȡ��ǰFrame����һFrame, ��ʱҪȷ��queue����������2��Frame */
// ������ʲôʱ����ã��������϶����� NULL
Frame* AVPacketQueue::frame_queue_peek_next(FrameQueue *f) {
	return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

void AVPacketQueue::packet_queue_about(PacketQueue *q) {
	std::unique_lock<std::mutex> lock(*q->mutex_t);
	q->abort_request = 1;       // �����˳�
	q->cond_t->notify_one();    //�ͷ�һ�������ź�
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
		// �ͷŶ�vp->frame�е����ݻ����������ã�ע�ⲻ���ͷ�frame������
		frame_queue_unref_item(vp);
		// �ͷ�vp->frame����
		av_frame_free(&vp->frame);
	}
	std::unique_lock<std::mutex> lock(*f->mutex_t);
	f->cond->notify_one();
}

int AVPacketQueue::frame_queue_nb_remaining(FrameQueue *f)
{
	return f->size - f->rindex_shown;	// ע������ΪʲôҪ��ȥf->rindex_shown
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
		if (!(f->queue[i].frame = av_frame_alloc())) // ����AVFrame�ṹ��
			return AVERROR(ENOMEM);
	return 0;
}

int AVPacketQueue::frame_video_frame_put(AVFrame* src_frame, double pts, double duration, int64_t pos, int serial) {
	Frame* vp;

	if (!(vp = frame_queue_peek_writable(&m_video_frame_queue)))
		return -1;

	// ִ�е��ⲽ˵�Ѿ���ȡ���˿�д���Frame
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

	av_frame_move_ref(vp->frame, src_frame); // ��src����������ת�Ƶ�dst�У�����λsrc��
	frame_queue_push(&m_video_frame_queue);   // ����д����λ��
	return 0;
}

int AVPacketQueue::frame_audio_frame_put(AVFrame* src_frame, double pts, double duration, int64_t pos, int serial) {
	Frame *af;

	// ��ȡ��дframe
	if (!(af = frame_queue_peek_writable(&m_audio_frame_queue)))
		return -1;

	// 3. ����Frame������FrameQueue
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
	c->pts = pts;                      /* ��ǰ֡��pts */
	c->last_updated = time;            /* �����µ�ʱ�䣬ʵ�����ǵ�ǰ��һ��ϵͳʱ�� */
	c->pts_drift = c->pts - time;      /* ��ǰ֡pts��ϵͳʱ��Ĳ�ֵ������������������ߵĲ�ֵӦ���ǱȽϹ̶��ģ���Ϊ���߶�����ʱ��Ϊ��׼������������ */
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
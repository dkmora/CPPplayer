#include "QtFFplay.h"

QtFFplay::QtFFplay(QObject* parent /* = nullptr */) :QObject(parent) {
	init_logger("rtmp_push.log", S_INFO);
	m_avplayer = new AVPlayer;
	m_videoplayer = new PlayVideo;
}

QtFFplay::~QtFFplay() {
	Stop();
	delete m_videoplayer;
	delete m_avplayer;
}

RET_CODE QtFFplay::Play(QString filename) {
	if (filename.isEmpty())
		return RET_FAIL;

	m_mutex.lock();
	int ret = 0;
	ret = m_avplayer->play(filename.toStdString(), [=] {
		// 开始播放音频
		if (m_audioplayer == nullptr
			&& m_avplayer->IsHaveAudioStream()) {
			m_audioplayer = new PlayAudio(m_avplayer, this);
		}
		// 开始渲染视频
		if (m_avplayer->IsHaveVideoStream()) {
			m_videoplayer->StartRender(m_avplayer);
		}
	});
	m_mutex.unlock();
	return RET_OK;
}

void QtFFplay::Stop() {
	qDebug() << "QtFFplay::Stop()";
	m_mutex.lock();
	// 结束渲染视频
	m_videoplayer->StopRender();
	// 结束播放音频
	if (m_audioplayer != nullptr) {
		delete m_audioplayer;
		m_audioplayer = nullptr;
	}
	m_avplayer->stop();
	m_mutex.unlock();
}

void QtFFplay::Pause() {
	m_avplayer->Pause();
}

void QtFFplay::Seek(int64_t pos, int64_t rel, int seek_by_bytes) {
	m_avplayer->Seek(pos, rel, seek_by_bytes);
}

void QtFFplay::Rate(double speed) {
	m_avplayer->Rate(speed);
}
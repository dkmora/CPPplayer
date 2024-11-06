#pragma once
#include "qtffplay_global.h"
#include<QString>
#include<QObject>
#include <mutex>
#include "AVPlayer.h"
#include "PlayAudio.h"
#include "PlayVideo.h"

class QTFFPLAY_EXPORT QtFFplay : public QObject
{
	Q_OBJECT
public:
	QtFFplay(QObject* parent = nullptr);
	~QtFFplay();

	RET_CODE Play(QString filename);
	PlayVideo* GetVideoView() { return m_videoplayer; }
	void Stop();
	void Pause();
	int64_t GetDuration() { return m_avplayer->get_file_duration(); }
	void Seek(int64_t pos, int64_t rel, int seek_by_bytes);
	void Rate(double speed);

	void setMediaPlayerEventHandler(MediaPlayerEventHandler* eventHandler) { m_avplayer->setMediaPlayerEventHandler(eventHandler); }

private:
	std::mutex m_mutex;
	AVPlayer* m_avplayer = nullptr;     // 音视频解复用
	PlayAudio* m_audioplayer = nullptr; // 音频播放器
	PlayVideo* m_videoplayer = nullptr; // 视频播放器
};
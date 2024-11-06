#pragma once

#include <QtWidgets/QWidget>
#include "ui_QtMediaPlayer.h"
#include "QtFFplay.h"
#include "MediaPlayerEvent.h"
#include <QDebug>
#include <QHBoxLayout>

class QtMediaPlayer;
class CMediaPlayerEvent : public MediaPlayerEventHandler {
public:
	CMediaPlayerEvent(QtMediaPlayer& mediaplayer) :
		m_pInstance(mediaplayer)
	{}

	virtual void onPlayerStateChange(MediaPlayerState state, MediaPlayerError error) {
		qDebug() << "onPlayerStateChange " << "state:" << state << " " << "error:" << error;
	}

private:
	QtMediaPlayer& m_pInstance;
};

class QtMediaPlayer : public QMainWindow
{
    Q_OBJECT

public:
    QtMediaPlayer(QWidget *parent = Q_NULLPTR);

private slots:
	bool StartPublish();
	void StopPublish();
	void Pause();
	void sloSliderSeek(int value);
	void sloSliderRate(int value);

private:
    Ui::QPublishStreamClass ui;
	QtFFplay* m_ffplay;
	std::unique_ptr<MediaPlayerEventHandler> m_MediaEventHandler;
};


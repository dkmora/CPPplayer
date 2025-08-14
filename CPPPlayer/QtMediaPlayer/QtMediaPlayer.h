#pragma once

#include <QtWidgets/QWidget>
#include "ui_QtMediaPlayer.h"
#include "QtFFplay.h"
#include "MediaPlayerEvent.h"
#include "./Export/ExportVideoWidget.h"
#include "./Montage/EnginePlayer.h"
#include "./Common/ColorfulTextWidget.h"
#include <QDebug>
#include <QHBoxLayout>

class QtMediaPlayer;
class CMediaPlayerEvent : public MediaPlayerEventHandler {
public:
	CMediaPlayerEvent(QtMediaPlayer& mediaplayer) :
		m_pInstance(mediaplayer)
	{}

	virtual void onPlayerStateChange(MediaPlayerState state, MediaPlayerError error);

private:
	QtMediaPlayer& m_pInstance;
};

class QtMediaPlayer : public QMainWindow
{
    Q_OBJECT

public:
    QtMediaPlayer(QWidget *parent = Q_NULLPTR);
	~QtMediaPlayer();

protected:
	//void dragEnterEvent(QDragEnterEvent* event) Q_DECL_OVERRIDE;
	//void dragMoveEvent(QDragMoveEvent* event) Q_DECL_OVERRIDE;
	//void dropEvent(QDropEvent* event) Q_DECL_OVERRIDE;
	bool nativeEvent(const QByteArray& eventType, void* message, long* result);

public slots:
	bool StartPublish();
	void StopPublish();
	void Pause();
	void sloSliderSeek(int value);
	void sloSliderRate(int value);
	void sloSliderVolume(int value);

signals:
	void sigDragFileEvent(QString path);

private:
    Ui::QPublishStreamClass ui;
	QtFFplay* m_ffplay = nullptr;
	EnginePlayer m_engine_player;
	std::unique_ptr<MediaPlayerEventHandler> m_MediaEventHandler;

	ExportVideoWidget* m_export_video_widget = nullptr;
	ColorfulLabel* m_color_text = nullptr;

	int m_engineplay_index = 0;
};

extern QtMediaPlayer* g_MediaPlayer;


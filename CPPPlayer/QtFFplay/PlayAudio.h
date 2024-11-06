#ifndef TESTAUDIO_H
#define TESTAUDIO_H

#include <QWidget>
#include "AVPlayer.h"
#include <QDebug>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QApplication>
#include <QFile>
#include <QThread>
#include <SDL.h>

#define AUDIO_MAX_CALLBACKS_PER_SEC 30

static SDL_AudioDeviceID audio_dev;

class PlayAudio : public QObject
{
	Q_OBJECT

public:
	explicit PlayAudio(AVPlayer* avplayer, QObject *parent = nullptr);
	~PlayAudio();

private:
	/* prepare a new audio buffer */
    /**
    * @brief sdl_audio_callback
    * @param opaque    指向user的数据
    * @param stream    拷贝PCM的地址
    * @param len       需要拷贝的长度
    */
	static void sdl_audio_callback(void *opaque, Uint8 *stream, int len);

private:
	AudioParams audio_hw_params;

private:
	AVPlayer* m_avplayer = NULL;
	QFile* m_file_write_pcm;

	uint64_t  callbacktime;
	uint64_t  audio_callback_time;
	uint8_t* m_data_pcm = nullptr;
	size_t m_size_pcm;
};

#endif // TESTAUDIO_H

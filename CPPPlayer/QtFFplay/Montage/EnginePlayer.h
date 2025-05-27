#pragma once

#include <QObject>
#include <thread>
#include <QDebug>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QApplication>
#include <QFile>
#include <QThread>
#include <SDL.h>

#include "qtffplay_global.h"
#include "OpenGlWidget.h"
#include "ffmpegbase.h"

class AnalyzeFrameEngine;

class QTFFPLAY_EXPORT EnginePlayVideo : public OpenGLWidget
{
public:
    EnginePlayVideo();
    ~EnginePlayVideo();

    void StartRender(AnalyzeFrameEngine* engine);
    void StopRender();

private:
    void sloTimerRender();

private:
    AnalyzeFrameEngine* m_frame_engine = nullptr;
    std::thread* m_timer_render = nullptr;
    bool m_render = false;
};

class EnginePlayAudio : public QObject
{
    Q_OBJECT
public:
    explicit EnginePlayAudio(AnalyzeFrameEngine* engine, QObject* parent = nullptr);
    ~EnginePlayAudio();

    void setEngine(AnalyzeFrameEngine* engine);

private:
    /* prepare a new audio buffer */
    /**
    * @brief sdl_audio_callback
    * @param opaque    指向user的数据
    * @param stream    拷贝PCM的地址
    * @param len       需要拷贝的长度
    */
    static void sdl_audio_callback(void* opaque, Uint8* stream, int len);

private:
    AudioParams audio_hw_params;

private:
    AnalyzeFrameEngine* m_frame_engine = nullptr;

    QFile* m_file_write_pcm;

    uint64_t  callbacktime;
    uint64_t  audio_callback_time;
    uint8_t*  m_data_pcm = nullptr;
    size_t    m_size_pcm = 0;
};

class QTFFPLAY_EXPORT EnginePlayer : public QObject
{
    Q_OBJECT
public:
    EnginePlayer(QObject* parent = nullptr);
    ~EnginePlayer();

    void Play(AnalyzeFrameEngine* engine);

    EnginePlayVideo* GetVideoView() { return m_videoplayer; }



private:

    EnginePlayVideo* m_videoplayer = nullptr; // 视频播放器
    EnginePlayAudio* m_audioplayer = nullptr; // 音频播放器
};
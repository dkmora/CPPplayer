#pragma once

#include "ui_VideoListWidget.h"
#include "Montage/AnalyzeFrame.h"
#include "Montage/MuxerVideo.h"

#include <QDragEnterEvent>
#include <QMimeData>
#include <QTextStream>

class VideoListWidget : public QWidget
{
    Q_OBJECT
public:
    VideoListWidget(QString filename, QWidget* parent = nullptr);
    ~VideoListWidget();

    AnalyzeFrameEngine* getAnalyzeFrameEngine() { return &m_analyzeframe; }

private:
    Ui::VideoListWidget ui;
    AnalyzeFrameEngine m_analyzeframe;
    int m_timeline = 0;
};
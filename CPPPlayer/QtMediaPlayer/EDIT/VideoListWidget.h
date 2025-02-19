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
    VideoListWidget(QWidget* parent = nullptr);
    ~VideoListWidget();

private:
    Ui::VideoListWidget ui;
    AnalyzeFrame m_analyzeframe;
    MuxerVideo   m_muxer_video;
    int m_timeline = 0;
};
#pragma once

#include "ui_VideoListWidget.h"
#include "Montage/AnalyzeFrame.h"
#include "Common/DragListWidget.h"

#include <QDragEnterEvent>
#include <QMimeData>
#include <QTextStream>

class VideoListWidget : public DragItemWidget
{
    Q_OBJECT
public:
    VideoListWidget(QString fileName, QWidget* parent = nullptr);
    ~VideoListWidget();

    //AnalyzeFrameEngine* getAnalyzeFrameEngine() { return &m_analyzeframe; }

private:
    Ui::VideoListWidget ui;
    int m_timeline = 0;
};
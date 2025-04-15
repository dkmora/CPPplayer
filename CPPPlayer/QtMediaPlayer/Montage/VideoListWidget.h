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
    VideoListWidget(AFMsg afMsg, QWidget* parent = nullptr);
    ~VideoListWidget();

    void setDuration(int64_t duration);

    void LoadKeyFrame(const std::list<AVFrame*> framelist);

private:
    void Init();

private:
    Ui::VideoListWidget ui;
    AFMsg m_afMsg;
    int m_timeline = 0;
};
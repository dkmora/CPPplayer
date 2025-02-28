#pragma once

#include "ui_VideoScaleWidget.h"
#include "VideoListWidget.h"
#include "Common/FrameLess.h"
#include <QList>

class VideoScaleWidget : public QWidget
{
    Q_OBJECT
public:
    VideoScaleWidget(QWidget* parent = nullptr);
    ~VideoScaleWidget();

private:
    Ui::VideoScaleWidget ui;
    QList<VideoListWidget*> m_video_list;
};
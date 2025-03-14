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

    QList<AFMsg> getAFMuxerMsg() { return ui.m_draglistwidget->GetItemDataList(); }

private:
    Ui::VideoScaleWidget ui;
    QList<VideoListWidget*> m_video_list;
};
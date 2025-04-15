#pragma once

#include "ui_VideoScaleWidget.h"
#include "VideoListWidget.h"
#include "Common/FrameLess.h"
#include <QMap>
#include <QSharedPointer>

class VideoScaleWidget : public QWidget
{
    Q_OBJECT
public:
    VideoScaleWidget(QWidget* parent = nullptr);
    ~VideoScaleWidget();

    void saveAFMuxerMsg();


    void sloFrameLessWidth(int _width, int _index);

private:
    Ui::VideoScaleWidget ui;
    QMap<QString, QSharedPointer<FrameLess>> m_frameless_map;

};
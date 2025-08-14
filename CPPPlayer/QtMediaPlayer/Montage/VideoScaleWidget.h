#pragma once

#include "ui_VideoScaleWidget.h"
#include "VideoListWidget.h"
#include "Common/FrameLess.h"
#include "VideoMainSeek.h"
#include <QMap>
#include <QSharedPointer>
#include <QTimer>

class VideoScaleWidget : public QWidget
{
    Q_OBJECT
public:
    VideoScaleWidget(QWidget* parent = nullptr);
    ~VideoScaleWidget();

    void saveAFMuxerMsg();

    void sloFrameLessWidth(int _width, int _index);

private:
    void showMainSeek();

private slots:
    virtual void resizeEvent(QResizeEvent* event);
    virtual void showEvent(QShowEvent* event);

private:
    Ui::VideoScaleWidget ui;
    QMap<QString, QSharedPointer<FrameLess>> m_frameless_map;
    VideoMainSeek* m_VideoMainSeek = nullptr;
    QTimer* m_timer_all_seek = nullptr;
};
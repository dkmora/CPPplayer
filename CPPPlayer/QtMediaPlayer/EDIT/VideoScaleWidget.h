#pragma once

#include "ui_VideoScaleWidget.h"
#include "VideoListWidget.h"
#include "Common/FrameLess.h"

#include <QTextEdit>

class VideoScaleWidget : public QWidget
{
    Q_OBJECT
public:
    VideoScaleWidget(QWidget* parent = nullptr);
    ~VideoScaleWidget();

protected:
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);

private:
    Ui::VideoScaleWidget ui;
    VideoListWidget* m_VideoListWidget;
    FrameLess* fremeLess;
    QTextEdit* m_text_edit = nullptr;
};
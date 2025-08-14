#pragma once
#include "ui_VideoMainSeek.h"

class VideoMainSeek : public QWidget
{
    Q_OBJECT
public:
    VideoMainSeek(QWidget* parent = NULL);
    ~VideoMainSeek();

    void setSeekValue(int value);

private:
    virtual void paintEvent(QPaintEvent* event);

private:
    Ui::VideoMainSeek ui;
    int m_seek_value = 0;
};
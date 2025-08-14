#include "VideoMainSeek.h"
#include <QPainter>

VideoMainSeek::VideoMainSeek(QWidget* parent/* = NULL*/) : QWidget(parent)
{
    ui.setupUi(this);

    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    //setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
}

VideoMainSeek::~VideoMainSeek()
{

}

void VideoMainSeek::setSeekValue(int value)
{
    m_seek_value = value;
    update();
}

void VideoMainSeek::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    QRect rect = geometry();
    int c_x = rect.width() / 2;
    int c_y = rect.height() / 2;

    int _progress = this->width() * (double)m_seek_value / 100;
    QPen pen(QColor("#929292"), 3); 
    painter.setPen(pen);
    painter.drawLine(_progress, 0, _progress, c_y * 2);
}
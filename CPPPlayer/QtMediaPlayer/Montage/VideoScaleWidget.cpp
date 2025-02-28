#include "VideoScaleWidget.h"
#include "gQGlobal.h"

VideoScaleWidget::VideoScaleWidget(QWidget* parent/* = nullptr*/) : QWidget(parent)
{
    ui.setupUi(this);

    connect(g_MediaPlayer, &QtMediaPlayer::sigDragFileEvent, this, [=](QString path) {
        QHBoxLayout* hly = new QHBoxLayout(ui.m_scrollArea_mixed);
        for (int i = 0; i < 1; i++) {
            VideoListWidget* widget = new VideoListWidget(path, ui.m_scrollArea_mixed);
            hly->setSpacing(0);
            hly->addWidget(widget);
            FrameLess *fremeLess = new FrameLess(widget);
            m_video_list.push_back(widget);
        }
    });
}

VideoScaleWidget::~VideoScaleWidget()
{

}
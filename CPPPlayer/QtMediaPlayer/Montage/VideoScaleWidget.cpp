#include "VideoScaleWidget.h"
#include "gQGlobal.h"

VideoScaleWidget::VideoScaleWidget(QWidget* parent/* = nullptr*/) : QWidget(parent)
{
    ui.setupUi(this);
    //emit sigDragFileEvent("D:\sound_in_sync_test.mp4");
    //connect(g_MediaPlayer, &QtMediaPlayer::sigDragFileEvent, this, [=](QString path) {
        //QHBoxLayout* hly = new QHBoxLayout(ui.m_draglistwidget);
        //for (int i = 0; i < 1; i++) {
        //    VideoListWidget* widget_1 = new VideoListWidget("D:\\sound_in_sync_test.mp4");
        //    //hly->setSpacing(0);
        //    hly->addWidget(widget_1);
        //    FrameLess *fremeLess_1 = new FrameLess(widget_1);
        //    m_video_list.push_back(widget_1);
        ////}
        //
        //    VideoListWidget* widget_2 = new VideoListWidget("D:\\jingluo.mp4");
        //    //hly->setSpacing(0);
        //    hly->addWidget(widget_2);
        //    FrameLess *fremeLess_2 = new FrameLess(widget_2);
        //    m_video_list.push_back(widget_2);
    //});

    VideoListWidget* item1 = new VideoListWidget("D:\\sound_in_sync_test.mp4");
    VideoListWidget* item2 = new VideoListWidget("D:\\jingluo.mp4");
    VideoListWidget* item3 = new VideoListWidget("D:\\media.mp4");

    ui.m_draglistwidget->AddWidgetItem(item1);
    ui.m_draglistwidget->AddWidgetItem(item2);
    ui.m_draglistwidget->AddWidgetItem(item3);

    connect(ui.m_draglistwidget, &DragListWidget::sigInsertDragItem, this, [=] (AFMsg afMsg){
        auto newItem = new VideoListWidget(afMsg.fileName);
        ui.m_draglistwidget->InsertWidgetItem(afMsg.insertrow, newItem);
    });
}

VideoScaleWidget::~VideoScaleWidget()
{

}

QList<AFMsg> VideoScaleWidget::getAFMuxerMsg() { 
    ui.m_draglistwidget->GetItemDataList(); 
}
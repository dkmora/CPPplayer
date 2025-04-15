#include "VideoScaleWidget.h"
#include "gQGlobal.h"
#include <QScrollBar>

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

    //ui.m_draglistwidget->setViewMode(QListView::IconMode);  // 设置为图标模式

    AFMsg afMsg;
    afMsg.fileName = "D:\\sound_in_sync_test.mp4";
    afMsg.afId = generateUniqueID("D:\\sound_in_sync_test.mp4");
    VideoListWidget* item1 = new VideoListWidget(afMsg);
    QSharedPointer<FrameLess> fremeLess_1 =  QSharedPointer<FrameLess>(new FrameLess(0, item1));
    m_frameless_map.insert(afMsg.afId, fremeLess_1);
    connect(m_frameless_map[afMsg.afId].data(), &FrameLess::sigFrameLessWidth, this, &VideoScaleWidget::sloFrameLessWidth);

    afMsg.fileName = "D:\\jingluo.mp4";
    afMsg.afId = generateUniqueID("D:\\jingluo.mp4");
    VideoListWidget* item2 = new VideoListWidget(afMsg);
    QSharedPointer<FrameLess> fremeLess_2 = QSharedPointer<FrameLess>(new FrameLess(1, item2));
    m_frameless_map.insert(afMsg.afId, fremeLess_2);
    connect(m_frameless_map[afMsg.afId].data(), &FrameLess::sigFrameLessWidth, this, &VideoScaleWidget::sloFrameLessWidth);

    afMsg.fileName = "D:\\media.mp4";
    afMsg.afId = generateUniqueID("D:\\media.mp4");
    VideoListWidget* item3 = new VideoListWidget(afMsg);
    QSharedPointer<FrameLess> fremeLess_3 = QSharedPointer<FrameLess>(new FrameLess(2, item3));
    m_frameless_map.insert(afMsg.afId, fremeLess_3);
    connect(m_frameless_map[afMsg.afId].data(), &FrameLess::sigFrameLessWidth, this, &VideoScaleWidget::sloFrameLessWidth);

    ui.m_draglistwidget->AddWidgetItem(item1);
    ui.m_draglistwidget->AddWidgetItem(item2);
    ui.m_draglistwidget->AddWidgetItem(item3);

    connect(ui.m_draglistwidget, &DragListWidget::sigInsertDragItem, this, [=] (AFMsg afMsg){
        disconnect(m_frameless_map[afMsg.afId].data(), &FrameLess::sigFrameLessWidth, this, &VideoScaleWidget::sloFrameLessWidth);
        m_frameless_map.remove(afMsg.afId);

        auto newItem = new VideoListWidget(afMsg);
        newItem->resize(QSize(afMsg.itemWidth, 55));
        ui.m_draglistwidget->InsertWidgetItem(afMsg.insertrow, newItem);
        QSharedPointer<FrameLess> fremeLess = QSharedPointer<FrameLess>(new FrameLess(afMsg.insertrow, newItem));
        m_frameless_map.insert(afMsg.afId, fremeLess);
        connect(fremeLess.data(), &FrameLess::sigFrameLessWidth, this, &VideoScaleWidget::sloFrameLessWidth);

        // 顺序改变重新设置index
        int index = 0;
        auto list = ui.m_draglistwidget->GetItemDataList();
        for (const auto& item : list) {
            m_frameless_map[item.afId]->setIndex(index);
            index++;
        }
    });
}

VideoScaleWidget::~VideoScaleWidget()
{

}

void VideoScaleWidget::saveAFMuxerMsg() {
    vAnalyzeManager->setExportSeq(ui.m_draglistwidget->GetItemDataList());
}

void VideoScaleWidget::sloFrameLessWidth(int _width, int _index)
{
    VideoListWidget* _drag_item = static_cast<VideoListWidget*>(ui.m_draglistwidget->itemWidget(ui.m_draglistwidget->item(_index)));
    int max_width = _drag_item->maximumWidth();
    if (max_width < _width)
        return;

    QListWidgetItem* item = ui.m_draglistwidget->item(_index);
    item->setSizeHint(QSize(_width, 55));

    int _duration = _width / 10;
    _drag_item->setDuration((int64_t)_duration * 1000000);

    auto afmsg = _drag_item->getDropData();
    afmsg.itemWidth = _width;
    _drag_item->setDropData(afmsg);

    auto engine = vAnalyzeManager->getAnalyzeEngine(afmsg.afId);
    engine->setEndTime(_duration);
}
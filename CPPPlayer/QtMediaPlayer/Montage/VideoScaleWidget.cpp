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
    afMsg.fileName = "D:\\jingluo.mp4";
    afMsg.afId = generateUniqueID("D:\\jingluo.mp4");
    VideoListWidget* item1 = new VideoListWidget(afMsg);
    QSharedPointer<FrameLess> fremeLess_1 = QSharedPointer<FrameLess>(new FrameLess(0, item1));
    m_frameless_map.insert(afMsg.afId, fremeLess_1);
    connect(m_frameless_map[afMsg.afId].data(), &FrameLess::sigFrameLessWidth, this, &VideoScaleWidget::sloFrameLessWidth);

    afMsg.fileName = "D:\\sound_in_sync_test.mp4";
    afMsg.afId = generateUniqueID("D:\\sound_in_sync_test.mp4");
    VideoListWidget* item2 = new VideoListWidget(afMsg);
    QSharedPointer<FrameLess> fremeLess_2 = QSharedPointer<FrameLess>(new FrameLess(1, item2));
    m_frameless_map.insert(afMsg.afId, fremeLess_2);
    connect(m_frameless_map[afMsg.afId].data(), &FrameLess::sigFrameLessWidth, this, &VideoScaleWidget::sloFrameLessWidth);

    /*
    afMsg.fileName = "D:\\media.mp4";
    afMsg.afId = generateUniqueID("D:\\media.mp4");
    VideoListWidget* item3 = new VideoListWidget(afMsg);
    QSharedPointer<FrameLess> fremeLess_3 = QSharedPointer<FrameLess>(new FrameLess(2, item3));
    m_frameless_map.insert(afMsg.afId, fremeLess_3);
    connect(m_frameless_map[afMsg.afId].data(), &FrameLess::sigFrameLessWidth, this, &VideoScaleWidget::sloFrameLessWidth);*/

    ui.m_draglistwidget->AddWidgetItem(item1);
    ui.m_draglistwidget->AddWidgetItem(item2);
    //ui.m_draglistwidget->AddWidgetItem(item3);

    connect(ui.m_draglistwidget, &DragListWidget::sigInsertDragItem, this, [=](AFMsg afMsg) {
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

    int _mod_duration = _drag_item->getFileDuration() / 1000000 * (double)_width / max_width;
    _drag_item->modDuration((int64_t)_mod_duration * 1000000);

    auto afmsg = _drag_item->getDropData();
    afmsg.itemWidth = _width;
    _drag_item->setDropData(afmsg);

    auto engine = vAnalyzeManager->getAnalyzeEngine(afmsg.afId);
    engine->setEndTime(_mod_duration); 

    qDebug() << "endTime:" << engine->getEndTime();
}

void VideoScaleWidget::showMainSeek()
{
    if (!m_VideoMainSeek)
    {
        m_VideoMainSeek = new VideoMainSeek(ui.widgetAllFrame);
        m_timer_all_seek = new QTimer(this);
        m_timer_all_seek->setInterval(10);

        connect(ui.slider_allseek, &QSlider::sliderPressed, this, [=]() {
            m_timer_all_seek->start();
            });

        connect(ui.slider_allseek, &QSlider::sliderReleased, this, [=]() {
            m_timer_all_seek->stop();
            });

        connect(m_timer_all_seek, &QTimer::timeout, this, [=] {
            int _value = ui.slider_allseek->value();
            if (_value >= 99)
            {
                int barValue = ui.m_draglistwidget->getHorizontalScrollBar();
                barValue += 10;
                ui.m_draglistwidget->setHorizontalScrollBar(barValue);
            }
            else if (_value <= 1) {
                int barValue = ui.m_draglistwidget->getHorizontalScrollBar();
                barValue -= 10;
                ui.m_draglistwidget->setHorizontalScrollBar(barValue);
            }

            int hbar_value = ui.m_draglistwidget->getHorizontalScrollBar();

            int totalwidth = 0;
            auto itemList = ui.m_draglistwidget->GetItemDataList();
            for (int i = 0; i < itemList.size(); i++) {
                //qDebug() << "itemList.at(i).itemWidth = " << itemList.at(i).itemWidth;
                totalwidth += itemList.at(i).itemWidth;
            }

            // draglistwidget 当前滑动的位置
            int dragValue = ui.m_draglistwidget->getHorizontalScrollBar();
            qDebug() << "dragValue = " << dragValue;
            dragValue /= 10;

            if (_value >= 99 && dragValue > 0) {
                dragValue += _value;
                //dragValue /= 10
            }
            else if (dragValue > 0 && _value > 0 && _value < 99) {
                dragValue = dragValue - _value;
            }
            else {
                dragValue = _value;
            }
            qDebug() << "seek value = " << dragValue;

            });

        connect(ui.slider_allseek, &QSlider::valueChanged, this, [=](int value) {
            m_VideoMainSeek->setSeekValue(value);

            ////总时长
            //int total_duration = 0;
            //std::shared_ptr<AnalyzeFrameEngine> engine;
            //for (const auto& item : vAnalyzeManager->getExportSeq()) {
            //    engine = vAnalyzeManager->getAnalyzeEngine(item.afId);
            //    total_duration += engine->get_file_duration();
            //    break;
            //}

            //double incr, pos, frac;
            //double x = 100.0f;
            //int64_t ts;
            //int ns, hh, mm, ss;
            //int tns, thh, tmm, tss;
            //tns = total_duration / 1000000LL;
            //thh = tns / 3600;
            //tmm = (tns % 3600) / 60;
            //tss = (tns % 60);
            //frac = (double)value / 100;
            //ns = frac * tns;
            //hh = ns / 3600;
            //mm = (ns % 3600) / 60;
            //ss = (ns % 60);
            ////av_log(NULL, AV_LOG_INFO, "Seek to %2.0f%% (%2d:%02d:%02d) of total duration (%2d:%02d:%02d)       \n", frac * 100, hh, mm, ss, thh, tmm, tss);
            //ts = frac * total_duration;
            ////if (cur_stream->ic->start_time != AV_NOPTS_VALUE) // 是否指定播放起始时间
            ////	ts += cur_stream->ic->start_time;
            //engine->Seek(ts, 0, 0);

            });
    }
    m_VideoMainSeek->setGeometry(0, 0, ui.widgetAllFrame->width(), ui.widgetAllFrame->height());
 }

void VideoScaleWidget::resizeEvent(QResizeEvent* event)
{
    showMainSeek();
}

void VideoScaleWidget::showEvent(QShowEvent* event)
{
    showMainSeek();
}
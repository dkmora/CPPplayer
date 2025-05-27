#include "VideoListWidget.h"
#include <QImage>
#include "gQGlobal.h"

VideoListWidget::VideoListWidget(AFMsg afMsg, QWidget* parent /*= nullptr*/) : 
    m_afMsg(afMsg),
    DragItemWidget(parent)
{
    ui.setupUi(this);
    //setAttribute(Qt::WA_TransparentForMouseEvents, true);

    std::weak_ptr<AnalyzeFrameEngine> ptr_af = vAnalyzeManager->getAnalyzeEngine(m_afMsg.afId);
    if(ptr_af.expired()){
        auto ptr = std::make_shared<AnalyzeFrameEngine>();
        int engine_size = vAnalyzeManager->getEngineSize();
        ptr->setMuxerIndex(engine_size);
        vAnalyzeManager->addAnalyzeEngine(m_afMsg.afId, ptr);
        ptr->startAnalyze(m_afMsg.fileName.toStdString());
        ptr_af = ptr;
    }

    setDropData(m_afMsg);
    vAnalyzeManager->addExportSeq(m_afMsg);

    //文件名
    ui.labfilename->setText(ptr_af.lock()->get_file_name().c_str());
    //文件总时长
    int64_t duration = ptr_af.lock()->get_file_duration();
    m_file_duration = duration;
    modDuration(duration);

    auto list = ptr_af.lock()->getIFrameList();
    //关键帧显示
    LoadKeyFrame(list);

    // 关键帧数即控件长度
    setMaximumWidth(list.size() * 50);
    resize(list.size() * 50, 55);

    // 结束时间为文件时长 单位秒
    qint64 totalSeconds = duration / 1000000;
    ptr_af.lock()->setEndTime(totalSeconds);

    connect(ui.m_starttime_scroll->horizontalScrollBar(), &QScrollBar::valueChanged, this, [=](int value) {
            int64_t startTime = 0;
            startTime = m_file_duration / 1000000 * (double)value / m_frame_long;

            auto afMsg = getDropData();
            auto engine = vAnalyzeManager->getAnalyzeEngine(afMsg.afId);
            engine->setStartTime(startTime);
            seek(startTime, engine);

            qDebug() << "startTime:" << startTime;

            //engine->setEndTime(_mod_duration); // TODO:拖动滚动条也需要设置
        });
}

VideoListWidget::~VideoListWidget()
{

}

void VideoListWidget::modDuration(int64_t duration)
{
    int file_duration = duration / 1000000;
    qint64 minutes = file_duration / 60;
    qint64 seconds = file_duration % 60;
    QString duration_text = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    ui.labduration->setText(duration_text);
}

void VideoListWidget::LoadKeyFrame(const std::list<AVFrame*> framelist)
{
    auto list = framelist;
    QHBoxLayout* vly = new QHBoxLayout(ui.scrollAreaWidget);
    for (auto value : list) {
        QLabel* label = new QLabel;
        label->setFixedSize(50, 40);
        QImage temp = QImage(value->data[0], value->width, value->height, value->width * 4, QImage::Format_RGB32);
        if (temp.isNull()) { continue; }
        QPixmap pixmap = QPixmap::fromImage(temp);
        pixmap = pixmap.scaled(label->size());
        label->setPixmap(pixmap);
        vly->addWidget(label);
        m_frame_long += label->width();
    }
    vly->setMargin(0);
    vly->setSpacing(0);
}

void VideoListWidget::seek(int value, std::shared_ptr<AnalyzeFrameEngine> engine) {
    double incr, pos, frac;
    double x = 100.0f;
    int64_t ts;
    int ns, hh, mm, ss;
    int tns, thh, tmm, tss;
    tns = m_file_duration / 1000000LL;
    thh = tns / 3600;
    tmm = (tns % 3600) / 60;
    tss = (tns % 60);
    frac = (double)value / 100;
    ns = frac * tns;
    hh = ns / 3600;
    mm = (ns % 3600) / 60;
    ss = (ns % 60);
    //av_log(NULL, AV_LOG_INFO, "Seek to %2.0f%% (%2d:%02d:%02d) of total duration (%2d:%02d:%02d)       \n", frac * 100, hh, mm, ss, thh, tmm, tss);
    ts = frac * m_file_duration;
    //if (cur_stream->ic->start_time != AV_NOPTS_VALUE) // 是否指定播放起始时间
    //	ts += cur_stream->ic->start_time;
    engine->Seek(ts, 0, 0);
}

void VideoListWidget::Init()
{
}


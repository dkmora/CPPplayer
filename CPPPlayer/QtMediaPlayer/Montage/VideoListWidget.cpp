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
    //时长
    int64_t duration = ptr_af.lock()->get_file_duration();
    setDuration(duration);

    // 秒数即长度
    qint64 totalSeconds = duration / 1000000;
    setMaximumWidth(totalSeconds * 10);
    resize(totalSeconds * 10, 55);

    //关键帧显示
    auto list = ptr_af.lock()->getIFrameList();
    LoadKeyFrame(list);
}

VideoListWidget::~VideoListWidget()
{

}

void VideoListWidget::setDuration(int64_t duration)
{
    //时长
    qint64 totalSeconds = duration / 1000000;
    qint64 minutes = totalSeconds / 60;
    qint64 seconds = totalSeconds % 60;
    QString file_duration = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    ui.labduration->setText(file_duration);
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
    }
    vly->setMargin(0);
    vly->setSpacing(0);
}

void VideoListWidget::Init()
{

}


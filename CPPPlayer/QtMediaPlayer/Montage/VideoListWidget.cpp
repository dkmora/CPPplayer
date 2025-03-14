#include "VideoListWidget.h"
#include <QImage>
#include "gQGlobal.h"

VideoListWidget::VideoListWidget(QString fileName, QWidget* parent /*= nullptr*/) : DragItemWidget(parent)
{
    ui.setupUi(this);
    //setAttribute(Qt::WA_TransparentForMouseEvents, true);
    std::weak_ptr<AnalyzeFrameEngine> ptr_af = vAnalyzeManager->getAnalyzeEngine(fileName);
    if(ptr_af.expired()){
        auto ptr = std::make_shared<AnalyzeFrameEngine>();
        int engine_size = vAnalyzeManager->getEngineSize();
        ptr->setMuxerIndex(engine_size);
        vAnalyzeManager->addAnalyzeEngine(fileName, ptr);
        ptr->startAnalyze(fileName.toStdString());
        ptr_af = ptr;
    }

    AFMsg afMsg;
    afMsg.fileName = fileName;
    setDropData(afMsg);

    //文件名
    ui.labfilename->setText(ptr_af.lock()->get_file_name().c_str());
    //时长
    qint64 totalSeconds = ptr_af.lock()->get_file_duration() / 1000000;
    qint64 minutes = totalSeconds / 60;
    qint64 seconds = totalSeconds % 60;
    QString file_duration = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    ui.labduration->setText(file_duration);
    //关键帧
    auto list = ptr_af.lock()->getIFrameList();
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

VideoListWidget::~VideoListWidget()
{

}


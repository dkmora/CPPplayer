#include "VideoListWidget.h"
#include <QImage>
#include "gQGlobal.h"

VideoListWidget::VideoListWidget(QString filename, QWidget* parent /*= nullptr*/) : QWidget(parent)
{
    ui.setupUi(this);

    //connect(g_MediaPlayer, &QtMediaPlayer::sigDragFileEvent, this, [=](QString path) {
        m_analyzeframe.startAnalyze(filename.toStdString());

        ui.labfilename->setText(m_analyzeframe.get_file_name().c_str());

        // 微秒转换为秒
        qint64 totalSeconds = m_analyzeframe.get_file_duration() / 1000000;

        // 计算分钟和秒
        qint64 minutes = totalSeconds / 60;
        qint64 seconds = totalSeconds % 60;

        // 格式化成 "mm:ss" 格式
        QString file_duration = QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))  // 分钟部分，保证两位
            .arg(seconds, 2, 10, QChar('0')); // 秒部分，保证两位

        ui.labduration->setText(file_duration);

        auto list = m_analyzeframe.getIFrameList();

        QHBoxLayout* vly = new QHBoxLayout(ui.scrollAreaWidget);
        for (auto value : list) {
            QLabel* label = new QLabel;
            label->setFixedSize(50, 40);
            QImage temp = QImage(value->data[0], value->width, value->height, value->width * 4, QImage::Format_RGB32);
            if (temp.isNull()) {
                continue;
            }

            QPixmap pixmap = QPixmap::fromImage(temp);
            pixmap = pixmap.scaled(label->size());
            label->setPixmap(pixmap);
            vly->addWidget(label);
        }
        vly->setMargin(0);
        vly->setSpacing(0);
    //    });
}

VideoListWidget::~VideoListWidget()
{

}


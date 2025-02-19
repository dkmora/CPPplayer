#include "VideoScaleWidget.h"

VideoScaleWidget::VideoScaleWidget(QWidget* parent/* = nullptr*/) : QWidget(parent)
{
    ui.setupUi(this);

    m_VideoListWidget = new VideoListWidget(ui.videolist);
    fremeLess = new FrameLess(m_VideoListWidget);

    m_text_edit = new QTextEdit(this);
    //m_text_edit->setAcceptDrops(false);
    setAcceptDrops(true);

    QVBoxLayout* mly = new QVBoxLayout(this);
    mly->addWidget(m_text_edit);
    mly->setSpacing(0);
    mly->setMargin(0);
}

VideoScaleWidget::~VideoScaleWidget()
{

}

void VideoScaleWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
    else
    {
        event->ignore();
    }
}

void VideoScaleWidget::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls())
    {
        QList<QUrl>urlList = mimeData->urls();
        QString fileName = urlList.at(0).toLocalFile();
        if (!fileName.isEmpty())
        {
            QFile file(fileName);
            if (!file.open(QIODevice::ReadOnly))return;
            QTextStream in(&file);
            //ui->textEdit->setText(in.readAll());
        }
    }
}
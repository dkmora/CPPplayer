#include "ExportVideoWidget.h"

ExportVideoWidget::ExportVideoWidget(QWidget* parent/* = nullptr*/) : QWidget(parent)
{
    ui.setupUi(this);

    // 分辨率
    ui.comboxResolution->addItem("480P");
    ui.comboxResolution->addItem("720P");
    ui.comboxResolution->addItem("1080P");
    ui.comboxResolution->setItemData(0, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxResolution->setItemData(1, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxResolution->setItemData(2, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxResolution->setCurrentIndex(1);

    //码率
    ui.comboxBitRate->addItem(QStringLiteral("更低"));
    ui.comboxBitRate->addItem(QStringLiteral("推荐"));
    ui.comboxBitRate->addItem(QStringLiteral("更高"));
    ui.comboxBitRate->setItemData(0, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxBitRate->setItemData(1, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxBitRate->setItemData(2, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxBitRate->setCurrentIndex(1);

    //编码
    ui.comboxVideoFormat->addItem("H.264");
    ui.comboxVideoFormat->setItemData(0, Qt::AlignCenter, Qt::TextAlignmentRole);

    //格式
    ui.comboxFileFormat->addItem("MP4");
    ui.comboxFileFormat->addItem("MOV");
    ui.comboxFileFormat->setItemData(0, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxFileFormat->setItemData(1, Qt::AlignCenter, Qt::TextAlignmentRole);

    //帧率
    ui.comboxFPS->addItem("24fps");
    ui.comboxFPS->addItem("30fps");
    ui.comboxFPS->addItem("50fps");
    ui.comboxFPS->addItem("60fps");
    ui.comboxFPS->setItemData(0, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxFPS->setItemData(1, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxFPS->setItemData(2, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxFPS->setItemData(3, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxFPS->setCurrentIndex(1);

    //音频导出格式
    ui.comboxAudioFormat->addItem("MP3");
    ui.comboxAudioFormat->addItem("WAV");
    ui.comboxAudioFormat->addItem("AAC");
    ui.comboxAudioFormat->addItem("FLAC");
    ui.comboxAudioFormat->setItemData(0, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxAudioFormat->setItemData(1, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxAudioFormat->setItemData(2, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxAudioFormat->setItemData(3, Qt::AlignCenter, Qt::TextAlignmentRole);

    connect(ui.btnExportvideo, &QPushButton::clicked, this, [=] { 
        close(); 
    });
    connect(ui.btnCancel, &QPushButton::clicked, this, [=] { close(); });
}

ExportVideoWidget::~ExportVideoWidget()
{

}

#include "ExportVideoWidget.h"
#include "gQGlobal.h"

ExportVideoWidget::ExportVideoWidget(QWidget* parent/* = nullptr*/) : QWidget(parent)
{
    ui.setupUi(this);

    ui.editTitle->setText("test");
    ui.editExportPath->setText("D:\\");

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
    ui.comboxFPS->addItem("24fps", 24);
    ui.comboxFPS->addItem("30fps", 30);
    ui.comboxFPS->addItem("50fps", 50);
    ui.comboxFPS->addItem("60fps", 60);
    ui.comboxFPS->setItemData(0, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxFPS->setItemData(1, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxFPS->setItemData(2, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxFPS->setItemData(3, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxFPS->setCurrentIndex(0);

    //音频导出格式
    ui.comboxAudioFormat->addItem("MP3");
    ui.comboxAudioFormat->addItem("WAV");
    ui.comboxAudioFormat->addItem("AAC");
    ui.comboxAudioFormat->addItem("FLAC");
    ui.comboxAudioFormat->setItemData(0, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxAudioFormat->setItemData(1, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxAudioFormat->setItemData(2, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui.comboxAudioFormat->setItemData(3, Qt::AlignCenter, Qt::TextAlignmentRole);

    connect(ui.btnExportvideo, &QPushButton::clicked, this, &ExportVideoWidget::ExportVideo);
    connect(ui.btnCancel, &QPushButton::clicked, this, [=] { close(); });
}

ExportVideoWidget::~ExportVideoWidget()
{

}

void ExportVideoWidget::ExportVideo()
{
    if (ui.editTitle->text().isEmpty())
    {
        QMessageBox::StandardButton result = QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请输入标题"), QMessageBox::Ok);
        return;
    }

    if (ui.editExportPath->text().isEmpty())
    {
        QMessageBox::StandardButton result = QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请选择路径"), QMessageBox::Ok);
        return;
    }

    QString _export_path = ui.editExportPath->text();
    QString _file_name = ui.editTitle->text();
    QString _file_format = ui.comboxFileFormat->currentText();
    int _fps = ui.comboxFPS->itemData(ui.comboxFPS->currentIndex()).toInt();

    int _width, _height = 0;
    if (ui.comboxResolution->currentText() == "480P") {
        _width = 720; _height = 480;
    }
    else if (ui.comboxResolution->currentText() == "720P") {
        _width = 1280; _height = 720;
    }
    else if (ui.comboxResolution->currentText() == "1080P") {
        _width = 1920; _height = 1080;
    }

    // start mutex..
    int64_t audio_time_base = AUDIO_TIME_BASE;
    int64_t video_time_base = VIDEO_TIME_BASE;
    double audio_pts = 0;
    double video_pts = 0;
    double audio_frame_duration = 1.0 * 1024 / 44100 * audio_time_base;
    double video_frame_duration = 1.0 / _fps * video_time_base;

    QString fill_name = QString("%1\\%2.%3").arg(_export_path).arg(_file_name).arg(_file_format);

    m_muxer_video.startMuxer(fill_name.toStdString(), _width, _height, _fps);
    for (int i = 0; i < vAnalyzeManager->getAnalyzeSize(); i++) {
        auto analyze_frame_Engine = vAnalyzeManager->getAnalyzeEngine(i);
        // 开始解码
        analyze_frame_Engine->startDecode(_width, _height);

        std::this_thread::sleep_for(std::chrono::seconds(3));  // 休眠 5 秒

        bool _push_video_finish = false;
        bool _push_audio_finish = false;
        while (1) {
            if (_push_video_finish && _push_audio_finish)
                break;

            printf("apts:%0.0lf vpts:%0.0lf\n", audio_pts / 1000, video_pts / 1000);

            // 重新编码
            if (audio_pts > video_pts && !_push_video_finish) {
                auto vframe = analyze_frame_Engine->getVidioDecode();
                if (vframe != NULL) {
                    m_muxer_video.pushYUV(vframe, video_pts);
                }
                else {
                    _push_video_finish = true;
                }
                video_pts += video_frame_duration;
            }
            else {
                auto aframe = analyze_frame_Engine->getAudioDecode();
                if (aframe != NULL) {
                    m_muxer_video.pushPCM(aframe, audio_pts);
                }
                else {
                    _push_audio_finish = true;
                }
                audio_pts += audio_frame_duration;
            }
        }
    }
    m_muxer_video.endMuxer();

    close();
}

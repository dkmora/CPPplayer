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
    //int _fps = ui.comboxFPS->itemData(ui.comboxFPS->currentIndex()).toInt();
    int _fps = 25; // 暂时只支持25fps视频编辑

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

    // start mutex...
    // 单位微妙
    int64_t audio_time_base = AUDIO_TIME_BASE;
    int64_t video_time_base = VIDEO_TIME_BASE;
    double audio_pts = 0;
    double video_pts = 0;
    double audio_frame_duration = 1.0 * 1024 / 44100 * audio_time_base;
    double video_frame_duration = 1.0 / _fps * video_time_base;

    QString fill_name = QString("%1\\%2.%3").arg(_export_path).arg(_file_name).arg(_file_format);

    //// TODO:滤镜
    //AVRational time_base = { 1, 25 };
    //AVRational sample_aspect_ratio = { 1, 1 };
    //CFFilter fpsfilter;
    //fpsfilter.initFilter(/*_width, _height*/720, 576, AV_PIX_FMT_YUV420P, time_base, sample_aspect_ratio);

    m_muxer_video.startMuxer(fill_name.toStdString() ,_width, _height, _fps);

    for (const auto& item : vAnalyzeManager->getExportSeq()) {
        auto engine = vAnalyzeManager->getAnalyzeEngine(item.afId);
        uint64_t end_time = engine->getEndTime() * 1000000;

        // 开始解码
        engine->startDecode(_width, _height);
        // TODO:改为非阻塞等待
        std::this_thread::sleep_for(std::chrono::seconds(3));  // 休眠 3 秒

        bool _push_video_finish = false;
        bool _push_audio_finish = false;
        //bool _push_video_frame = true;

        uint64_t _video_pts_end = 0;
        uint64_t _autio_pts_end = 0;

        while (1) {
            if (_push_video_finish && _push_audio_finish)
                break;

            // 重新编码
            if (audio_pts > video_pts && !_push_video_finish) {
                //int ret = 0; AVFrame* vframe = NULL;
                //if (_push_video_frame)
                //{
                //    vframe = analyze_frame_Engine->getVidioDecode();
                //    if (vframe != NULL) {
                //        int ret = fpsfilter.sendFrame(vframe);
                //        if (ret < 0) {
                //            continue;
                //        }
                //    }
                //}
                // 
                //push:
                //AVFrame* filger_frame = fpsfilter.receiveFrame(ret);
                //if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                //{
                //    if (vframe == NULL)
                //    {
                //        _push_video_frame = true;
                //    }
                //    continue;
                //}
                //if (filger_frame != NULL && ret == 0) {
                //    m_muxer_video.pushYUV(filger_frame, video_pts);
                //    video_pts += video_frame_duration;
                //    _push_video_frame = false;
                //    printf("pushYUV vpts:%0.0lf\n", video_pts / 1000);
                //    continue;
                //}
                auto vframe = engine->getVidioDecode();
                if (vframe != NULL && _video_pts_end <= end_time) {
                    m_muxer_video.pushYUV(vframe, video_pts);
                }
                else {
                    _push_video_finish = true;
                }
                printf("Video frame duration: %lf seconds\n", video_pts);
                video_pts += video_frame_duration;
                _video_pts_end += video_frame_duration;
                //printf("pushYUV vpts:%0.0lf\n", video_pts / 1000);
            }
            else {
                auto aframe = engine->getAudioDecode();
                if (aframe != NULL && _autio_pts_end <= end_time) {
                    m_muxer_video.pushPCM(aframe, audio_pts);
                }
                else {
                    _push_audio_finish = true;
                }
                printf("Audio frame total duration: %lf seconds\n", audio_pts);
                audio_pts += audio_frame_duration;
                _autio_pts_end += audio_frame_duration;
                //printf("pushPCM apts:%0.0lf\n", audio_pts / 1000);
            }
        }
    }
    m_muxer_video.endMuxer();
    close();
}

#include "VideoListWidget.h"
#include <QImage>

VideoListWidget::VideoListWidget(QWidget* parent /*= nullptr*/) : QWidget(parent)
{
    ui.setupUi(this);

    m_analyzeframe.startAnalyze("D:\\github\\bin\\Win32\\Debug\\sound_in_sync_test.mp4");
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

#define AUDIO_TIME_BASE 1000000
#define VIDEO_TIME_BASE 1000000

    connect(ui.btnExportVideo, &QPushButton::clicked, this, [=] {

        int64_t audio_time_base = AUDIO_TIME_BASE;
        int64_t video_time_base = VIDEO_TIME_BASE;
        double audio_pts = 0;
        double video_pts = 0;
        double audio_frame_duration = 1.0 * 1024 / 44100 * audio_time_base;
        double video_frame_duration = 1.0 / 25 * video_time_base;

        m_muxer_video.startMuxer("myxporttest.mp4", m_analyzeframe.get_video_width(), 
            m_analyzeframe.get_video_height(), m_analyzeframe.get_video_fps());

        size_t read_len = 0;
        AVPacket* packet = NULL;
        std::vector<AVPacket*> packets;

        bool _push_video_finish = false;
        bool _push_audio_finish = false;

        while (1) {
            if (_push_video_finish && _push_audio_finish)
                break;

            printf("apts:%0.0lf vpts:%0.0lf\n", audio_pts / 1000, video_pts / 1000);
            if (audio_pts > video_pts && !_push_video_finish) {
                auto vframe = m_analyzeframe.getVidioDecode();
                if (vframe != NULL) {
                    m_muxer_video.pushYUV(vframe, video_pts);
                }
                else {
                    _push_video_finish = true;
                }
                video_pts += video_frame_duration;
            }
            else{
                auto aframe = m_analyzeframe.getAudioDecode();
                if (aframe != NULL) {
                    m_muxer_video.pushPCM(aframe, audio_pts);
                }
                else {
                    _push_audio_finish = true;
                }
                audio_pts += audio_frame_duration;
            }
        }
        m_muxer_video.endMuxer();
        });
}

VideoListWidget::~VideoListWidget()
{

}



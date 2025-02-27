#include "VideoScaleWidget.h"
#include "gQGlobal.h"

VideoScaleWidget::VideoScaleWidget(QWidget* parent/* = nullptr*/) : QWidget(parent)
{
    ui.setupUi(this);

    connect(g_MediaPlayer, &QtMediaPlayer::sigDragFileEvent, this, [=](QString path) {
        QHBoxLayout* hly = new QHBoxLayout(ui.m_scrollArea_mixed);
        for (int i = 0; i < 2; i++) {
            VideoListWidget* widget = new VideoListWidget(path, ui.m_scrollArea_mixed);
            hly->setSpacing(0);
            hly->addWidget(widget);
            FrameLess *fremeLess = new FrameLess(widget);
            m_video_list.push_back(widget);
        }
    });

#define AUDIO_TIME_BASE 1000000
#define VIDEO_TIME_BASE 1000000
    connect(ui.btnExportVideo, &QPushButton::clicked, this, [=] {

        int64_t audio_time_base = AUDIO_TIME_BASE;
        int64_t video_time_base = VIDEO_TIME_BASE;
        double audio_pts = 0;
        double video_pts = 0;
        double audio_frame_duration = 1.0 * 1024 / 44100 * audio_time_base;
        double video_frame_duration = 1.0 / 25 * video_time_base;

        auto _engine = m_video_list.at(0)->getAnalyzeFrameEngine();
        m_muxer_video.startMuxer("myxporttest.mp4", 
            1280, 720, 60
   /*         _engine->get_video_width(),
            _engine->get_video_height(),
            _engine->get_video_fps()*/);

        for (int i = 0; i < m_video_list.size(); i++) {
            auto analyze_frame_Engine = m_video_list.at(i)->getAnalyzeFrameEngine();

            bool _push_video_finish = false;
            bool _push_audio_finish = false;

            while (1) {
                if (_push_video_finish && _push_audio_finish)
                    break;

                printf("apts:%0.0lf vpts:%0.0lf\n", audio_pts / 1000, video_pts / 1000);
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
        });
}

VideoScaleWidget::~VideoScaleWidget()
{

}
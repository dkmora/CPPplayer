#include "QtMediaPlayer.h"

QtMediaPlayer::QtMediaPlayer(QWidget* parent)
	: QMainWindow(parent),
	m_MediaEventHandler(new CMediaPlayerEvent(*this))
{
	ui.setupUi(this);
	connect(ui.m_btn_start_publishstream, &QPushButton::clicked, this, &QtMediaPlayer::StartPublish);
	connect(ui.m_btn_stop_publishstream, &QPushButton::clicked, this, &QtMediaPlayer::StopPublish);
	connect(ui.m_btn_pause, &QPushButton::clicked, this, &QtMediaPlayer::Pause);
	connect(ui.m_slider_seek, &QSlider::valueChanged, this, &QtMediaPlayer::sloSliderSeek);
	connect(ui.m_slider_playrate, &QSlider::valueChanged, this, &QtMediaPlayer::sloSliderRate);

	m_ffplay = new QtFFplay;
	m_ffplay->setMediaPlayerEventHandler(m_MediaEventHandler.get());
	ui.m_edit_address->setText("E://Program Files//JiJiDown//Download//testmovie.mp4");

	ui.m_slider_playrate->setMinimum(0);
	ui.m_slider_playrate->setMaximum(4);
	ui.m_slider_playrate->setValue(2);

	QHBoxLayout* mainlayout = new QHBoxLayout(ui.m_widget_video);
	mainlayout->addWidget(m_ffplay->GetVideoView());
	mainlayout->setMargin(0);
	mainlayout->setSpacing(0);
}

bool QtMediaPlayer::StartPublish() {
	QString filename = ui.m_edit_address->text();
	if (filename.isEmpty())
		return false;

	int ret = 0;
	ret = m_ffplay->Play(filename);
	ui.m_slider_seek->setValue(0);
	ui.m_slider_playrate->setValue(2);
	return RET_OK;
}

void QtMediaPlayer::StopPublish() {
	m_ffplay->Stop();
}

void QtMediaPlayer::Pause() {
	m_ffplay->Pause();
}

void QtMediaPlayer::sloSliderSeek(int value) {
	double incr, pos, frac;
	double x = 100.0f;
	int64_t ts;
	int ns, hh, mm, ss;
	int tns, thh, tmm, tss;
	tns = m_ffplay->GetDuration() / 1000000LL;
	thh = tns / 3600;
	tmm = (tns % 3600) / 60;
	tss = (tns % 60);
	frac = (double)value / 100;
	ns = frac * tns;
	hh = ns / 3600;
	mm = (ns % 3600) / 60;
	ss = (ns % 60);
	//av_log(NULL, AV_LOG_INFO, "Seek to %2.0f%% (%2d:%02d:%02d) of total duration (%2d:%02d:%02d)       \n", frac * 100, hh, mm, ss, thh, tmm, tss);
	ts = frac * m_ffplay->GetDuration();
	//if (cur_stream->ic->start_time != AV_NOPTS_VALUE) // 是否指定播放起始时间
	//	ts += cur_stream->ic->start_time;
	m_ffplay->Seek(ts, 0, 0);
}

void QtMediaPlayer::sloSliderRate(int value) {
	switch (value) {
	case 0:
		m_ffplay->Rate(0.5);
		break;
	case 1:
		m_ffplay->Rate(0.7);
		break;
	case 2:
		m_ffplay->Rate(1);
		break;
	case 3:
		m_ffplay->Rate(1.5);
		break;
	case 4:
		m_ffplay->Rate(2);
		break;
	}
}
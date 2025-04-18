#include "QtMediaPlayer.h"
#include "gQGlobal.h"

QtMediaPlayer* g_MediaPlayer = nullptr;

QtMediaPlayer::QtMediaPlayer(QWidget* parent)
	: QMainWindow(parent),
	m_MediaEventHandler(new CMediaPlayerEvent(*this))
{
	g_MediaPlayer = this;

	ui.setupUi(this);
	connect(ui.m_btn_start_publishstream, &QPushButton::clicked, this, &QtMediaPlayer::StartPublish);
	connect(ui.m_btn_stop_publishstream, &QPushButton::clicked, this, &QtMediaPlayer::StopPublish);
	connect(ui.m_btn_pause, &QPushButton::clicked, this, &QtMediaPlayer::Pause);
	connect(ui.m_slider_seek, &QSlider::valueChanged, this, &QtMediaPlayer::sloSliderSeek);
	connect(ui.m_slider_playrate, &QSlider::valueChanged, this, &QtMediaPlayer::sloSliderRate);
	connect(ui.m_slider_volume, &QSlider::valueChanged, this, &QtMediaPlayer::sloSliderVolume);

	//m_ffplay = new QtFFplay;
	//m_ffplay->setMediaPlayerEventHandler(m_MediaEventHandler.get());
	//ui.m_edit_address->setText("D:\\github\\bin\\Win32\\Debug\\sound_in_sync_test.mp4");

	ui.m_slider_volume->setMinimum(0);
	ui.m_slider_volume->setMaximum(100);
	ui.m_slider_volume->setValue(100);

	ui.m_slider_playrate->setMinimum(0);
	ui.m_slider_playrate->setMaximum(4);
	ui.m_slider_playrate->setValue(2);

	QHBoxLayout* mainlayout = new QHBoxLayout(ui.m_widget_video);
	mainlayout->addWidget(m_engine_player.GetVideoView());
	mainlayout->setMargin(0);
	mainlayout->setSpacing(0);

	// QMenu
	connect(ui.actionExport, &QAction::triggered, this, [=] {
		if (m_export_video_widget == nullptr)
		{
			m_export_video_widget = new ExportVideoWidget(this);
		}

		// 开始导出 获取视频拼接的顺序
		ui.main_seek_widget->saveAFMuxerMsg();

		auto region = m_export_video_widget->rect();
		region.moveCenter(this->rect().center());
		auto posX = region.x();
		auto posY = region.y();
		m_export_video_widget->move(posX, posY);
		m_export_video_widget->show();
	});
}

QtMediaPlayer::~QtMediaPlayer()
{
	//delete m_ffplay;
}

bool QtMediaPlayer::StartPublish() {
	int ret = 0;
	//ret = m_ffplay->Play(filename);
	for (const auto& item : vAnalyzeManager->getExportSeq()) {
		auto engine = vAnalyzeManager->getAnalyzeEngine(item.afId);
		engine->play([=] {
			m_engine_player.Play(engine.get());
	    });
	}

	//ui.m_slider_seek->setValue(0);
	//ui.m_slider_playrate->setValue(2);
	return RET_OK;
}

void QtMediaPlayer::StopPublish() {
	//m_ffplay->Stop();
}

void QtMediaPlayer::Pause() {
	//m_ffplay->Pause();
}

void QtMediaPlayer::sloSliderSeek(int value) {
	//double incr, pos, frac;
	//double x = 100.0f;
	//int64_t ts;
	//int ns, hh, mm, ss;
	//int tns, thh, tmm, tss;
	//tns = m_ffplay->GetDuration() / 1000000LL;
	//thh = tns / 3600;
	//tmm = (tns % 3600) / 60;
	//tss = (tns % 60);
	//frac = (double)value / 100;
	//ns = frac * tns;
	//hh = ns / 3600;
	//mm = (ns % 3600) / 60;
	//ss = (ns % 60);
	////av_log(NULL, AV_LOG_INFO, "Seek to %2.0f%% (%2d:%02d:%02d) of total duration (%2d:%02d:%02d)       \n", frac * 100, hh, mm, ss, thh, tmm, tss);
	//ts = frac * m_ffplay->GetDuration();
	////if (cur_stream->ic->start_time != AV_NOPTS_VALUE) // 是否指定播放起始时间
	////	ts += cur_stream->ic->start_time;
	//m_ffplay->Seek(ts, 0, 0);
}

void QtMediaPlayer::sloSliderRate(int value) {
	//switch (value) {
	//case 0:
	//	m_ffplay->Rate(0.5);
	//	break;
	//case 1:
	//	m_ffplay->Rate(0.7);
	//	break;
	//case 2:
	//	m_ffplay->Rate(1);
	//	break;
	//case 3:
	//	m_ffplay->Rate(1.5);
	//	break;
	//case 4:
	//	m_ffplay->Rate(2);
	//	break;
	//}
}

void QtMediaPlayer::sloSliderVolume(int value)
{
	//float fVolume = (float)value / 100;
	//m_ffplay->Volume(fVolume);
}


//void QtMediaPlayer::dragEnterEvent(QDragEnterEvent* event) //拖动文件到窗口，触发
//{
//	if (event->mimeData()->hasUrls())
//	{
//		event->acceptProposedAction(); //事件数据中存在路径，方向事件
//	}
//	else
//	{
//		event->ignore();
//	}
//}
//
//void QtMediaPlayer::dragMoveEvent(QDragMoveEvent* event) //拖动文件到窗口移动文件，触发
//{
//	qDebug() << "123";
//}
//
//void QtMediaPlayer::dropEvent(QDropEvent* event) //拖动文件到窗口释放文件，触发
//{
//	const QMimeData* mimeData = event->mimeData();
//	if (mimeData->hasUrls())
//	{
//		QList<QUrl> urls = mimeData->urls();
//		QString fileName = urls.at(0).toLocalFile();
//		m_text_edit->setText(fileName);
//	}
//
//}

bool QtMediaPlayer::nativeEvent(const QByteArray& eventType, void* message, long* result) {
	if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
		MSG* pMsg = reinterpret_cast<MSG*>(message);
		if (pMsg->message == WM_DROPFILES) {
			HDROP hDropInfo = (HDROP)pMsg->wParam;
			wchar_t szFilePathName[_MAX_PATH] = { 0 };
			const UINT nNumOfFiles = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
			if (nNumOfFiles > 0) {
				// DragQueryFile第二个参数为拖入文件的索引
				DragQueryFile(hDropInfo, 0, szFilePathName, _MAX_PATH); //直接取第一个 入参UINT iFile  = 0
				const QString currentfile = QString::fromWCharArray(szFilePathName);
				// currentfile 为当前拖拽文件
				// OnDragFinished(currentfile);
				emit sigDragFileEvent(currentfile); 
			}
			DragFinish(hDropInfo);
		}
	}
	return false;
}

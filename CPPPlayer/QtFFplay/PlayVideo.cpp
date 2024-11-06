#include "PlayVideo.h"

PlayVideo::PlayVideo()
{

}

PlayVideo::~PlayVideo() {
	StopRender();
}

void PlayVideo::StartRender(AVPlayer* avplayer) {
	m_avplayer = avplayer;
	if (m_timer_render == nullptr) {
		int fps = m_avplayer->getFps();
		m_render = true;
		m_timer_render = new std::thread(&PlayVideo::sloTimerRender, this);
	}
}

void PlayVideo::StopRender() {
	m_render = false;
	m_avplayer = nullptr;
	if (m_timer_render != nullptr) {
		m_timer_render->join();
		delete m_timer_render;
		m_timer_render = nullptr;
	}
}

//#include <QDateTime>
void PlayVideo::sloTimerRender() {
	while (m_render) {
		uint8_t* idY = nullptr;
		uint8_t* idU = nullptr;
		uint8_t* idV = nullptr;
		int videoW = 0;
		int videoH = 0;
		auto videodecode = m_avplayer->getVidioDecode();
		if (videodecode == nullptr) {
			continue;
		}
		videodecode->getVideoYuv420Frame(&idY, &idU, &idV, videoW, videoH);
		if (idY == nullptr || idU == nullptr || idV == nullptr) {
			continue;
		}

		/*static uint64_t time1 = 0;
		uint64_t time2 = QDateTime::currentDateTime().toMSecsSinceEpoch();
		if (time1 == 0) {
			time1 = time2;
		}
		else {
			printf("Ê±¼ä²î : %d\n", time2 - time1);
			time1 = time2;
		}*/

		render(idY, idU, idV, videoW, videoH);
	}
	render(NULL, NULL, NULL, 0, 0);
}
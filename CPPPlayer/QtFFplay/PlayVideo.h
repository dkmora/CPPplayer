/*
* OpenGL  ”∆µ‰÷»æ¥∞ø⁄
*/

#pragma once
#include <thread>
#include "AVPlayer.h"
#include "OpenGlWidget.h"

class PlayVideo : public OpenGLWidget{
public:
	PlayVideo();
	~PlayVideo();

	void StartRender(AVPlayer* avplayer);
	void StopRender();

private:
	void sloTimerRender();

private:
	std::thread* m_timer_render = nullptr;
	AVPlayer* m_avplayer = nullptr;
	bool m_render = false;
};
#include "OpenGLWidget.h"
#include <QGraphicsView>
#include <QVBoxLayout>

int get_fps()
{
	static int fps = 0;

	static uint64_t lastTime = GetTickCount(); // ms
	static int frameCount = 0;

	++frameCount;

	uint64_t curTime = GetTickCount();
	if (curTime - lastTime > 1000)
	{
		fps = frameCount - 1;
		frameCount = 0;
		lastTime = curTime;
	}

	return fps;
}

OpenGLWidget::OpenGLWidget(QWidget* parent /* = nullptr */) :
	QOpenGLWidget(parent) {
	m_yuv420Render = new YUV420P_Render(this);
}

OpenGLWidget::~OpenGLWidget(){
	qDebug() << "~OpenGLWidget()";
}

void OpenGLWidget::initializeGL() {
	m_yuv420Render->initialize();
}

void OpenGLWidget::render(uchar* pidY, uchar* pidU, uchar* pidV, uint uvideoW, uint uvideoH)
{
	idY = pidY;
	idU = pidU;
	idV = pidV;
	videoW = uvideoW;
	videoH = uvideoH;
	update();
}

void OpenGLWidget::paintGL()
{
	m_yuv420Render->render(idY, idU, idV, videoW, videoH);
}

//void OpenGLWidget::resizeGL(int w, int h)
//{
//	m_yuv420Render->resizeGL(w, h);
//}
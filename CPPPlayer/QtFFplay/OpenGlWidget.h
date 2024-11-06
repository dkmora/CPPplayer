#pragma once
#include "YUV420P_Render.h"
#include <QTimer>
#include <QGraphicsView>
#include <QEvent>
#include <QObject>
#include <QThread>


class OpenGLWidget : public QOpenGLWidget{
	Q_OBJECT
public:
	OpenGLWidget(QWidget* parent = nullptr);
	~OpenGLWidget();

	void render(uchar* pidY, uchar* pidU, uchar* pidV, uint uvideoW, uint uvideoH);
	 
private:
	virtual void initializeGL();
	virtual void paintGL();
	//virtual void paintEvent(QPaintEvent *e);
	//virtual void resizeGL(int w, int h);

private:
	uint yuvType = 0;
	uchar* idY = nullptr;
	uchar* idU = nullptr;
	uchar* idV = nullptr; //自己创建的纹理对象ID，创建错误返回0
	uint videoW, videoH;
	uchar *yuvPtr = nullptr;
	YUV420P_Render* m_yuv420Render;
};


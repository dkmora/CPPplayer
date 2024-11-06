#ifndef YUV420P_RENDER_H
#define YUV420P_RENDER_H

#include <QObject>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>

class YUV420P_Render : protected QOpenGLFunctions
{
public:
	YUV420P_Render(QWidget* parent = nullptr);
	~YUV420P_Render();

	//初始化gl
	void initialize();
	//刷新显示
	void render(uchar* py, uchar* pu, uchar* pv, int width, int height);
	void render(uchar* ptr, int width, int height);
	void resizeGL(int width, int height);

private:
	//shader程序
	QOpenGLShaderProgram m_program;
	//shader中yuv变量地址
	GLuint m_textureUniformY, m_textureUniformU, m_textureUniformV;
	//创建纹理
	GLuint m_idy, m_idu, m_idv;
	//缓冲区对象
	GLuint m_renderbuffer;
	// 
	GLuint m_vertexAttr, m_textureAttr;
	QOpenGLBuffer m_vbo;
	QOpenGLFramebufferObject* m_frameBuf;
	QWidget* m_parent;

	int m_width, m_height = 0;
};

#endif // YUV420P_RENDER_H
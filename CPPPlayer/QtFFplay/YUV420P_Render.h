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

	//��ʼ��gl
	void initialize();
	//ˢ����ʾ
	void render(uchar* py, uchar* pu, uchar* pv, int width, int height);
	void render(uchar* ptr, int width, int height);
	void resizeGL(int width, int height);

private:
	//shader����
	QOpenGLShaderProgram m_program;
	//shader��yuv������ַ
	GLuint m_textureUniformY, m_textureUniformU, m_textureUniformV;
	//��������
	GLuint m_idy, m_idu, m_idv;
	//����������
	GLuint m_renderbuffer;
	// 
	GLuint m_vertexAttr, m_textureAttr;
	QOpenGLBuffer m_vbo;
	QOpenGLFramebufferObject* m_frameBuf;
	QWidget* m_parent;

	int m_width, m_height = 0;
};

#endif // YUV420P_RENDER_H
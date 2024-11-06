#include "YUV420P_Render.h"
#include <QDebug>
#include <QTimer>
#include <QSurfaceFormat>

#define ATTRIB_VERTEX 0
#define ATTRIB_TEXTURE 1

//���ݶ������������
//����
static const GLfloat ver[] = {
	-1.0f,-1.0f,
	-1.0f,+1.0f,
	+1.0f,+1.0f,
    +1.0f,-1.0f
	//        -1.0f,-1.0f,
	//        0.9f,-1.0f,
	//        -1.0f, 1.0f,
	//        0.9f,1.0f
};
//����
static const GLfloat tex[] = {
	0.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f
};

YUV420P_Render::YUV420P_Render(QWidget* parent) {
	m_parent = parent;
}

YUV420P_Render::~YUV420P_Render(){
}

//��ʼ��gl
void YUV420P_Render::initialize()
{
	qDebug() << "initializeGL";

	//��ʼ��opengl ��QOpenGLFunctions�̳У�����
	initializeOpenGLFunctions();

	//GPU������ɫ��
	constexpr char vsrc[] =
		"attribute vec4 vertex;\n"
		"attribute vec2 texCoord;\n"
		"varying vec2 textureOut;\n"
		"void main(void)\n"
		"{\n"
		"    gl_Position = vertex;\n"
		"    textureOut = texCoord;\n"
		"}\n";
	//GPUƬԪ��ɫ��
	constexpr char fsrc[] =
		"varying vec2 textureOut; \
uniform sampler2D tex_y; \
uniform sampler2D tex_u; \
uniform sampler2D tex_v; \
void main(void) \
{ \
vec3 yuv; \
vec3 rgb; \
yuv.x = 1.164 * (texture2D(tex_y, textureOut).r - 0.0625); \
yuv.y = texture2D(tex_u, textureOut).r - 0.5; \
yuv.z = texture2D(tex_v, textureOut).r - 0.5; \
            rgb.r = yuv.x + 1.596 * yuv.z;\
    rgb.g = yuv.x - 0.391 * yuv.y - 0.813 * yuv.z;\
        rgb.b = yuv.x + 2.018 * yuv.y; \
gl_FragColor = vec4(rgb, 1); \
}";

	//m_program����shader�������ƬԪ���ű�
	//ƬԪ�����أ�
	qDebug() << m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, fsrc);
	//����shader
	qDebug() << m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vsrc);

	//���ö���λ��
	m_program.bindAttributeLocation("vertexPosition", ATTRIB_VERTEX);
	//��������λ��
	m_program.bindAttributeLocation("textureCoordinate", ATTRIB_TEXTURE);

	//����shader
	qDebug() << "m_program.link() = " << m_program.link();
	qDebug() << "m_program.bind() = " << m_program.bind();

	//��shader��ȡ��ַ
	m_vertexAttr = m_program.attributeLocation("vertex");
	m_textureAttr = m_program.attributeLocation("texCoord");
	m_textureUniformY = m_program.uniformLocation("tex_y");
	m_textureUniformU = m_program.uniformLocation("tex_u");
	m_textureUniformV = m_program.uniformLocation("tex_v");

	//
	m_vbo.create();
	m_vbo.bind();
	int size = sizeof(ver) + sizeof(tex);
	m_vbo.allocate(size);
	m_vbo.write(0, ver, sizeof(ver));
	m_vbo.write(sizeof(ver), tex, sizeof(tex));
	m_vbo.release();

	//���ö���,�������鲢����
	//glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, ver);
	//glEnableVertexAttribArray(ATTRIB_VERTEX);
	//glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, tex);
	//glEnableVertexAttribArray(ATTRIB_TEXTURE);

	//glDisableVertexAttribArray(ATTRIB_VERTEX);
	//glDisableVertexAttribArray(ATTRIB_TEXTURE);

	//��������
	glGenTextures(1, &m_idy);
	glBindTexture(GL_TEXTURE_2D, m_idy);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &m_idu);
	glBindTexture(GL_TEXTURE_2D, m_idu);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &m_idv);
	glBindTexture(GL_TEXTURE_2D, m_idv);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
}

//ˢ����ʾ
void YUV420P_Render::render(uchar* py, uchar* pu, uchar* pv, int width, int height)
{
	if (py == Q_NULLPTR || pu == Q_NULLPTR || pv == Q_NULLPTR) {
		return;
	}
	//��Ƶ�߶�ʼ���봰�ڸ߶����
	if (height == 0) {
		return;
	}

	if (width >= height) { // ������Ӧ
		auto radio = (float)width / height;
		auto realH = m_parent->rect().height();
		auto realW = radio * realH;
		auto posX = (m_parent->rect().width() - realW) / 2;
		glViewport(posX, 0, realW, realH);
	}
	else { // �������
		auto radio = (float)height / width;
		auto realW = m_parent->rect().width();
		auto realH = radio * realW;
		auto posX = (m_parent->rect().height() - realH) / 2;
		glViewport(0, posX, realW, realH);
	}

	/*p = m_pBuf;
	std::copy((uchar*)p, (uchar*)p + y_size, py);
	p += y_size;
	std::copy((uchar*)p, (uchar*)p + y_size / 4, pu);
	p += y_size / 4;
	std::copy((uchar*)p, (uchar*)p + y_size / 4, pv);*/

	//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT);
    //��ʼ
	m_program.bind();
	m_program.enableAttributeArray(m_vertexAttr);
	m_program.enableAttributeArray(m_textureAttr);
	//ָ��buffer
	m_vbo.bind();
	m_program.setAttributeBuffer(m_vertexAttr, GL_FLOAT, 0, 2);
	m_program.setAttributeBuffer(m_textureAttr, GL_FLOAT, sizeof(ver), 2);
	m_vbo.release();

	// Y
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_idy);
	//�޸���������(�����ڴ�����)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, py);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, m_pBuf);
	//��shader ����
	//glUniform1i(m_textureUniformY, 0);
	m_program.setUniformValue(m_textureUniformY, 0);

	// U
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_idu);
	//�޸���������(�����ڴ�����)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pu);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, m_pBuf + width * height);
	//��shader ����
    //glUniform1i(m_textureUniformU, 1);
	m_program.setUniformValue(m_textureUniformU, 1);

	// V
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_idv);
	//�޸���������(�����ڴ�����)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pv);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, m_pBuf + width * height * 5 / 4);
	//��shader ����
	//glUniform1i(m_textureUniformV, 2);
	m_program.setUniformValue(m_textureUniformV, 2);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);
	m_program.disableAttributeArray(m_vertexAttr);
	m_program.disableAttributeArray(m_textureAttr);
	m_program.release();

	//delete[] m_pBuf;
}

void YUV420P_Render::render(uchar* ptr, int width, int height)
{
	if (ptr == Q_NULLPTR) {
		return;
	}
	//��Ƶ�߶�ʼ���봰�ڸ߶����
	if (height == 0) {
		return;
	}

	if (width >= height) { // ������Ӧ
		auto radio = (float)width / height;
		auto realH = m_parent->rect().height();
		auto realW = radio * realH;
		auto posX = (m_parent->rect().width() - realW) / 2;
		glViewport(posX, 0, realW, realH);
	}
	else { // �������
		auto radio = (float)height / width;
		auto realW = m_parent->rect().width();
		auto realH = radio * realW;
		auto posX = (m_parent->rect().height() - realH) / 2;
		glViewport(0, posX, realW, realH);
	}

	//��ʼ
	m_program.bind();
	m_program.enableAttributeArray(m_vertexAttr);
	m_program.enableAttributeArray(m_textureAttr);
	//ָ��buffer
	m_vbo.bind();
	m_program.setAttributeBuffer(m_vertexAttr, GL_FLOAT, 0, 2);
	m_program.setAttributeBuffer(m_textureAttr, GL_FLOAT, sizeof(ver), 2);
	m_vbo.release();

	// Y
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_idy);
	//�޸���������(�����ڴ�����)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, ptr);
	//��shader ����
	//glUniform1i(m_textureUniformY, 0);
	m_program.setUniformValue(m_textureUniformY, 0);

	// U
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_idu);
	//�޸���������(�����ڴ�����)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, ptr + width * height);
	//��shader ����
	//glUniform1i(m_textureUniformU, 1);
	m_program.setUniformValue(m_textureUniformU, 1);

	// V
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_idv);
	//�޸���������(�����ڴ�����)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, ptr + width * height * 5 / 4);
	//��shader ����
	//glUniform1i(m_textureUniformV, 2);
	m_program.setUniformValue(m_textureUniformV, 2);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);
	m_program.disableAttributeArray(m_vertexAttr);
	m_program.disableAttributeArray(m_textureAttr);
	m_program.release();
}

void YUV420P_Render::resizeGL(int width, int height)
{
	//auto radio = (float)width / height;
	//auto realH = height;
	//auto realW = radio * realH;
	//auto posX = (width - realW) / 2;
	//glViewport(posX, 0, realW, realH);
	//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT);
}
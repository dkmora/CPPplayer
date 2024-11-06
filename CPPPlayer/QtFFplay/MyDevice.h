#ifndef MYDEVICE_H
#define MYDEVICE_H

#include <QIODevice>
#include "AVDecoder.h"
#include <QFile>
#include <QTimer>

class MyDevice : public QIODevice
{
public:
	MyDevice(cvpublish::AVDecoder* audio_decoder, QByteArray pcm); //�������󴫵�pcm����
	~MyDevice();

	qint64 readData(char *data, qint64 maxlen); //����ʵ�ֵ��麯��
	qint64 writeData(const char *data, qint64 len); //���Ǹ����麯���� ���ò�ʵ��

private:
	QFile* pcm_file;
	QByteArray data_pcm; //���pcm����
	int        len_written; //��¼��д������ֽ�
	cvpublish::AVDecoder* m_audio_decoder;

	int64_t callbacktime = 0;
	int64_t audio_callback_time;
};

#endif // MYDEVICE_H
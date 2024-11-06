#ifndef MYDEVICE_H
#define MYDEVICE_H

#include <QIODevice>
#include "AVDecoder.h"
#include <QFile>
#include <QTimer>

class MyDevice : public QIODevice
{
public:
	MyDevice(cvpublish::AVDecoder* audio_decoder, QByteArray pcm); //创建对象传递pcm数据
	~MyDevice();

	qint64 readData(char *data, qint64 maxlen); //重新实现的虚函数
	qint64 writeData(const char *data, qint64 len); //它是个纯虚函数， 不得不实现

private:
	QFile* pcm_file;
	QByteArray data_pcm; //存放pcm数据
	int        len_written; //记录已写入多少字节
	cvpublish::AVDecoder* m_audio_decoder;

	int64_t callbacktime = 0;
	int64_t audio_callback_time;
};

#endif // MYDEVICE_H
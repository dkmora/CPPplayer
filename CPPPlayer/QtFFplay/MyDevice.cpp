
#include "MyDevice.h"

#include <QDebug>
#include <QDateTime>
#include <QThread>

MyDevice::MyDevice(cvpublish::AVDecoder* audio_decoder, QByteArray pcm) : data_pcm(pcm), m_audio_decoder(audio_decoder)
{
	this->open(QIODevice::WriteOnly); // Ϊ�˽��QIODevice::read (QIODevice): device not open.
	len_written = 0;

	pcm_file = new QFile("test_44100_s16_c2.pcm");
	if (pcm_file->open(QIODevice::WriteOnly)) {
		qDebug() << "open test_44100_s16_c2.pcm seccess";
	}
}

MyDevice::~MyDevice()
{
	this->close();
}


// dataΪ���������ݻ�������ַ�� maxlenΪ��������������ܴ�ŵ��ֽ���.
qint64 MyDevice::readData(char *data, qint64 maxlen)
{
	return 0;

	if (callbacktime == 0) {
		callbacktime = QDateTime::currentMSecsSinceEpoch();
	}
	else {
		audio_callback_time = QDateTime::currentMSecsSinceEpoch();
		qDebug() << "call_back_time" << audio_callback_time - callbacktime;
		callbacktime = QDateTime::currentMSecsSinceEpoch();
	}

	uint8_t* data_pcm = nullptr;
	size_t size_pcm = 0;
	m_audio_decoder->getAudioFrame(&data_pcm, size_pcm);

	if (data_pcm && size_pcm) {
		//pcm_file->write((const char*)data_pcm, size_pcm);
		memcpy(data, data_pcm, size_pcm); //��Ҫ���ŵ�pcm���ݴ���������������.
		len_written += size_pcm; //�����Ѳ��ŵ����ݳ���.
		return size_pcm;
	}
	return size_pcm;

	// ���Դ���
	//if (len_written >= data_pcm.size())
	//	return 0;
	//int len;

	////����δ���ŵ����ݵĳ���.
	//len = (len_written + maxlen) > data_pcm.size() ? (data_pcm.size() - len_written) : maxlen;

	//memcpy(data, data_pcm.data() + len_written, len); //��Ҫ���ŵ�pcm���ݴ���������������.
	//len_written += len; //�����Ѳ��ŵ����ݳ���.

	//return len;
}

qint64 MyDevice::writeData(const char *data, qint64 len)
{
	return len;
}
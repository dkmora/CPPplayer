#include "AudioSource.h"

namespace cvpublish {

AudioSource::AudioSource() {

}

AudioSource::~AudioSource() {

}

RET_CODE AudioSource::Init(const Properties& properties) {

	RET_CODE ret;
	return ret;
}

void AudioSource::Loop() {
	LogInfo("into audio loop");
}

int AudioSource::openSaveAudioFile(const char* filename) {
	m_audio_save_fp = fopen(filename, "wb");
	return 0;
}

int AudioSource::saveFile(uint8_t* data, size_t size) {
	if (data == nullptr)
		return -1;

	fwrite(data, 1, size, m_audio_save_fp);
}

int AudioSource::closeSaveAudioFile() {
	if (m_audio_save_fp)
		fclose(m_audio_save_fp);
	return 0;
}


}
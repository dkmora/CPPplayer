#include "VideoSource.h"

namespace cvpublish 
{

VideoSource::VideoSource() {
}

VideoSource::~VideoSource() {

}

RET_CODE VideoSource::Init(const Properties& properties) {
	RET_CODE ret;

	m_is_desktop = properties.GetProperty("video_source", 0);
	m_str_filename = properties.GetProperty("video_file", "720x480_25fps.mp4");
	m_video_test = properties.GetProperty("video_test", 1);

	m_x = properties.GetProperty("x", 0);
	m_y = properties.GetProperty("y", 0);
	m_width = properties.GetProperty("width", 1920);
	m_height = properties.GetProperty("height", 1080);
	m_fps = properties.GetProperty("fps", 30);
	m_frame_duration = 1000.0 / m_fps;

	if (m_video_test) {
		openSaveVideoFile("720x480_25fps_420p.yuv");
	}

	if (!m_is_desktop) {
		/*if (openFile(m_str_filename.c_str()) == 0) {
			ret = m_video_coder.VideoDecode(m_str_filename);
		}
		else {
			LogError("openFile %s failed", m_str_filename.c_str());
			ret = RET_FAIL;
		}*/
	}
	else { // ²É¼¯×ÀÃæ

	}

	return ret;
}

void VideoSource::Loop() {
	LogInfo("into vudio loop");

	while (1) {
		uint8_t* y = nullptr;
		uint8_t* u = nullptr;
		uint8_t* v = nullptr;
		int width, height = 0;

		/*if (m_video_coder.getVideoFrame(&y, &u, &v, width, height) >= 0) {
			saveFile(y, u, v, width, height);
		}
		else {
			break;
		}*/
	}
}

int VideoSource::openFile(const char *file_name)
{
	m_video_fp = fopen(file_name, "rb");
	if (!m_video_fp)
	{
		return -1;
	}
	return 0;
}

int VideoSource::closeYuvFile()
{
	if (m_video_fp)
		fclose(m_video_fp);
	return 0;
}

int VideoSource::openSaveVideoFile(const char* filename) {
	m_video_save_fp = fopen(filename, "wb");
	return 0;
}

int VideoSource::saveFile(uint8_t* y, uint8_t* u, uint8_t* v, int width, int height) {
	if (y == nullptr || u == nullptr || v == nullptr || width <= 0 || height <= 0)
		return -1;

	int y_size = width * height;
	fwrite(y, 1, y_size, m_video_save_fp);
	fwrite(u, 1, y_size / 4, m_video_save_fp);
	fwrite(v, 1, y_size / 4, m_video_save_fp);
	return 0;
}

int VideoSource::closeSaveVideoFile() {
	if (m_video_save_fp)
		fclose(m_video_save_fp);
	return 0;
}


}
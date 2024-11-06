#pragma once
#include <iostream>
#include <functional>
#include "commonlooper.h"
#include "AVDecoder.h"

namespace cvpublish {

	class VideoSource : public CommonLooper {

	public:
		VideoSource();
		virtual ~VideoSource();

		RET_CODE Init(const Properties& properties);
		virtual void Loop();

		void AddCallback(std::function<void(uint8_t*, int32_t)> callable_object)
		{
			m_callable_object = callable_object;
		}

	private:
		int openFile(const char *file_name);
		int closeYuvFile();

	private: // 测试：输出视频数据到文件
		int m_video_test = 0;
		FILE* m_video_save_fp = NULL;
		int openSaveVideoFile(const char* filename);
		int saveFile(uint8_t* y, uint8_t* u, uint8_t* v, int width, int height);
		int closeSaveVideoFile();

	private:
		bool m_is_desktop = false;
		std::string m_str_filename;
		FILE* m_video_fp = NULL;
		//AVDecoder m_video_coder;

	private:
		int m_x = 0;
		int m_y = 0;
		int m_width = 0;
		int m_height = 0;
		int m_fps = 0;
		int m_bit = 0;
		double m_frame_duration = 40;

	private:
		std::function<void(uint8_t*, int32_t)> m_callable_object = nullptr;
	};
}
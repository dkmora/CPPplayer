#pragma once
#include <iostream>
#include <functional>
#include "commonlooper.h"
#include "AVDecoder.h"

namespace cvpublish {

	class AudioSource : public CommonLooper {

	public:
		AudioSource();
		virtual ~AudioSource();

		RET_CODE Init(const Properties& properties);
		virtual void Loop();

		void AddCallback(std::function<void(uint8_t*, int32_t)> callable_object)
		{
			m_callable_object = callable_object;
		}

	private: // 测试：输出音频数据到文件
		int openSaveAudioFile(const char* filename);
		int saveFile(uint8_t* data, size_t size);
		int closeSaveAudioFile();

		int m_audio_test = 0;
		FILE* m_audio_save_fp = NULL;

	private:
		bool m_is_desktop = false;
		std::string m_str_filename;
		FILE* m_video_fp = NULL;
		//AVDecoder m_video_coder;

	private:
		std::function<void(uint8_t*, int32_t)> m_callable_object = nullptr;
	};
}

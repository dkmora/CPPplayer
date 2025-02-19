#pragma once
#include <vector>
#include <string>

class PAGEngineCallback {
public:
	virtual void OnPagPlayEnd() = 0;
};

class IPPPAGEngine
{
public:
	virtual void* initEngine(unsigned hwnd, void* pag_engine_callback_, int width, int height) = 0;

	virtual void setFallbackFontPaths(std::vector<std::string> fontPaths) = 0;

	virtual void loadFile(const std::string& filePath, void* pag_engine_) = 0;

	virtual void resize(const int width, const int height, void* pag_engine_) = 0;

	virtual void flush(void * pag_engine_) = 0;

	virtual void deleteEngine(void* ptr) = 0;
};

namespace pp
{
	__declspec(dllexport) IPPPAGEngine* pagEngine();
	__declspec(dllexport) void destroypEngine();
}
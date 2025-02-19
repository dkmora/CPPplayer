#pragma once

#include "include/ppglobal.h"
#include "pag/ipppagengine.h"


class PPPAGEngineImpl : public QObject, public IPPPAGEngine
{
	Q_OBJECT

    PPPAGEngineImpl();
    ~PPPAGEngineImpl();

public:
	static PPPAGEngineImpl* instance();
	static void destroy();

public:
	virtual void* initEngine(unsigned hwnd, void* pag_engine_callback_, int width, int height);
	virtual void setFallbackFontPaths(std::vector<std::string> fontPaths);
	virtual void loadFile(const std::string& filePath, void* pag_engine_);
	virtual void resize(const int width, const int height, void* pag_engine_);
	virtual void flush(void* pag_engine_);
	virtual void deleteEngine(void* ptr);

private:
	void _init();
	HMODULE _load(const std::wstring& path);

private:
	HMODULE m_hModule = nullptr;
};

extern PPPAGEngineImpl* m_instance;

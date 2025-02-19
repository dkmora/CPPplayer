#include "pppagengineimpl.h"

#define log (qInfo() << QStringLiteral("[PPPAGEngineImpl]"))

PPPAGEngineImpl* m_instance = nullptr;

IPPPAGEngine* pp::pagEngine()
{
	return PPPAGEngineImpl::instance();
}

void pp::destroypEngine()
{
	PPPAGEngineImpl::destroy();
}

PPPAGEngineImpl* PPPAGEngineImpl::instance()
{
	//static PPPAGEngineImpl m_instance;
	//return &m_instance;
	if (m_instance == nullptr)
	{
		m_instance = new PPPAGEngineImpl;
	}
	return m_instance;
}

void PPPAGEngineImpl::destroy()
{
	if (m_instance != nullptr)
	{
		delete m_instance;
	}
	m_instance = nullptr;
}

PPPAGEngineImpl::PPPAGEngineImpl()
{
	_init();
}

PPPAGEngineImpl::~PPPAGEngineImpl()
{
	if (m_hModule)
	{
		typedef void (*_unLoad)();
		_unLoad pfn = (_unLoad)GetProcAddress(m_hModule, "unLoad");
		if (pfn)
		{
			pfn();
		}
	}
}

void* PPPAGEngineImpl::initEngine(unsigned hwnd, void* pag_engine_callback_, int width, int height)
{
	typedef void* (*_initEngine)(HWND, void*, int, int);
	_initEngine pfn = (_initEngine)GetProcAddress(m_hModule, "initEngine");
	if (pfn)
	{
		return pfn((HWND)hwnd, nullptr, width, height);
	}

	return nullptr;
}

void PPPAGEngineImpl::setFallbackFontPaths(std::vector<std::string> fontPaths)
{
	typedef void (*_setFallbackFontPaths)(std::vector<std::string>);
	_setFallbackFontPaths pfn = (_setFallbackFontPaths)GetProcAddress(m_hModule, "setFallbackFontPaths");
	if (pfn)
	{
		pfn(fontPaths);
	}
}

void PPPAGEngineImpl::loadFile(const std::string& filePath, void* pag_engine_)
{
	typedef void (*_loadFile)(const std::string&, void*);
	_loadFile pfn = (_loadFile)GetProcAddress(m_hModule, "loadFile");
	if (pfn)
	{
		pfn(filePath, pag_engine_);
	}
}

void PPPAGEngineImpl::resize(const int width, const int height, void* pag_engine_)
{
	typedef void (*_Resize)(int, int, void*);
	_Resize pfn = (_Resize)GetProcAddress(m_hModule, "resize");
	if (pfn)
	{
		pfn(width ,height, pag_engine_);
	}
}

void PPPAGEngineImpl::flush(void* pag_engine_)
{
	typedef void (*_flush)(void*);
	_flush pfn = (_flush)GetProcAddress(m_hModule, "flush");
	if (pfn)
	{
		pfn(pag_engine_);
	}
}

void PPPAGEngineImpl::deleteEngine(void* ptr)
{
	typedef void (*_deleteEngine)(void*);
	_deleteEngine pfn = (_deleteEngine)GetProcAddress(m_hModule, "deleteEngine");
	if (pfn)
	{
		pfn(ptr);
	}
}

void PPPAGEngineImpl::_init()
{
	std::wstring dirPath;
	dirPath.resize(MAX_PATH);
	::GetModuleFileName(::GetModuleHandle(nullptr), &dirPath[0], MAX_PATH);
	dirPath.resize(dirPath.rfind('\\') + 1);

	if (m_hModule = _load(dirPath + L"pagengine.dll"))
	{
		typedef void (*_load)();
		_load pfn = (_load)GetProcAddress(m_hModule, "load");
		if (pfn)
		{
			pfn();
		}
	}
}

HMODULE PPPAGEngineImpl::_load(const std::wstring& path)
{
	HMODULE hModule = GetModuleHandle(path.c_str());
	if (hModule)
	{
		return hModule;
	}

	hModule = LoadLibraryEx(path.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

	return hModule;
}

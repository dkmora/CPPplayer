#include "AnalyzeManager.h"

AnalyzeManager* g_AnalyzeManager = nullptr;

AnalyzeManager::AnalyzeManager()
{

}

AnalyzeManager::~AnalyzeManager()
{

}

AnalyzeManager* AnalyzeManager::getInstance()
{
    if (g_AnalyzeManager == nullptr)
    {
        g_AnalyzeManager = new AnalyzeManager();
    }
    return g_AnalyzeManager;
}

void AnalyzeManager::release()
{
    if (g_AnalyzeManager != nullptr)
    {
        delete g_AnalyzeManager;
        g_AnalyzeManager = nullptr;
    }
}

void AnalyzeManager::addAnalyzeEngine(AnalyzeFrameEngine* engine)
{
    m_analyze_frame_list.push_back(engine);
}

void AnalyzeManager::remoteAnalyzeEngine(AnalyzeFrameEngine* engine)
{
    m_analyze_frame_list.removeOne(engine);
}

int AnalyzeManager::getAnalyzeSize()
{
    return m_analyze_frame_list.size();
}

AnalyzeFrameEngine* AnalyzeManager::getAnalyzeEngine(int _index)
{
    return  m_analyze_frame_list.at(_index);
}
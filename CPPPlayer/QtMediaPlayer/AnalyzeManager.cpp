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

void AnalyzeManager::addAnalyzeEngine(QString fileName, std::shared_ptr<AnalyzeFrameEngine> engine)
{
    m_analyze_frame_map.insert(fileName, engine);
}

void AnalyzeManager::remoteAnalyzeEngine(QString fileName)
{
    m_analyze_frame_map.remove(fileName);
}

int AnalyzeManager::getEngineSize()
{
    return m_analyze_frame_map.size();
}

std::shared_ptr<AnalyzeFrameEngine> AnalyzeManager::getAnalyzeEngine(QString fileName)
{
    auto ptr = m_analyze_frame_map.find(fileName);
    if (ptr == m_analyze_frame_map.end())
    {
        return nullptr;
    }
    return  m_analyze_frame_map.find(fileName).value();
}

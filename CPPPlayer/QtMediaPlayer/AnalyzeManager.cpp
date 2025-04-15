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

void AnalyzeManager::addAnalyzeEngine(QString afId, std::shared_ptr<AnalyzeFrameEngine> engine)
{
    m_analyze_frame_map.insert(afId, engine);
}

void AnalyzeManager::remoteAnalyzeEngine(QString afId)
{
    m_analyze_frame_map.remove(afId);
}

int AnalyzeManager::getEngineSize()
{
    return m_analyze_frame_map.size();
}

std::shared_ptr<AnalyzeFrameEngine> AnalyzeManager::getAnalyzeEngine(QString afId)
{
    auto ptr = m_analyze_frame_map.find(afId);
    if (ptr == m_analyze_frame_map.end())
    {
        return nullptr;
    }
    return  m_analyze_frame_map.find(afId).value();
}

void AnalyzeManager::addExportSeq(const AFMsg& afMsg)
{
    for (const auto& item : m_aflist) {
        if (item.afId == afMsg.afId){
            return;
        }
    }
    m_aflist.append(afMsg);
}

void AnalyzeManager::setExportSeq(const QList<AFMsg>& list)
{
    m_aflist = list;
}
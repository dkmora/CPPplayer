#pragma once
#include "Montage/AnalyzeFrame.h"
#include <QList>
#include <QMap>
#include "Common/gStruct.h"

using AFEngineMap = QMap<QString, std::shared_ptr<AnalyzeFrameEngine>>;

class AnalyzeManager
{
private:
    AnalyzeManager();
    ~AnalyzeManager();

public:
    static AnalyzeManager* getInstance();
    static void release();

    void addAnalyzeEngine(QString fileName, std::shared_ptr<AnalyzeFrameEngine> engine);
    void remoteAnalyzeEngine(QString fileName);
    int  getEngineSize();
    std::shared_ptr<AnalyzeFrameEngine> getAnalyzeEngine(QString fileName);
    AFEngineMap getAFMap() { return m_analyze_frame_map; }

private:
    AFEngineMap m_analyze_frame_map;
};

extern AnalyzeManager* g_AnalyzeManager;
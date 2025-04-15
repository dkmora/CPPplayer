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

    // 剪辑引擎管理
    void addAnalyzeEngine(QString afId, std::shared_ptr<AnalyzeFrameEngine> engine);
    void remoteAnalyzeEngine(QString afId);
    int  getEngineSize();
    std::shared_ptr<AnalyzeFrameEngine> getAnalyzeEngine(QString afId);
    AFEngineMap getAFMap() { return m_analyze_frame_map; }

    // 视频裁剪的顺序
    void addExportSeq(const AFMsg& afMsg);
    void setExportSeq(const QList<AFMsg>& list);
    const QList<AFMsg>& getExportSeq() { return m_aflist; }

private:
    AFEngineMap m_analyze_frame_map;
    QList<AFMsg> m_aflist;
};

extern AnalyzeManager* g_AnalyzeManager;
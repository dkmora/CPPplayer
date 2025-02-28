#pragma once
#include "Montage/AnalyzeFrame.h"
#include <QList>

class AnalyzeManager
{
private:
    AnalyzeManager();
    ~AnalyzeManager();

public:
    static AnalyzeManager* getInstance();
    static void release();

    void addAnalyzeEngine(AnalyzeFrameEngine* engine);
    void remoteAnalyzeEngine(AnalyzeFrameEngine* engine);
    int  getAnalyzeSize();
    AnalyzeFrameEngine* getAnalyzeEngine(int _index);

private:
    QList<AnalyzeFrameEngine*> m_analyze_frame_list;
};

extern AnalyzeManager* g_AnalyzeManager;
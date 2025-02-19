#pragma once

#include <QWidget>
#include "include/ppglobal.h"
#include "ipppagengine.h"

class PPPAGWidget : public QWidget, public PAGEngineCallback
{
    Q_OBJECT

public:
    explicit PPPAGWidget(QWidget *parent = nullptr);
    ~PPPAGWidget();
    void paly(const QString& path, int msec = 20);
    void Resize(int width, int height);
    void Destory();
    void stop();

signals:
    void sigPagPlayEnd();

public:
    void OnPagPlayEnd();

private:
    QTimer m_timer;
    void* m_pEngine = nullptr;
};

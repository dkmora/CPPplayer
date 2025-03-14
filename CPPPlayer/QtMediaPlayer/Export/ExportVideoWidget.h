#pragma once
#include "ui_ExportVideoWidget.h"
#include "Montage/MuxerVideo.h"
#include "Filter/CFFilter.h"

class ExportVideoWidget : public QWidget
{
Q_OBJECT
public:
    ExportVideoWidget(QWidget* parent = nullptr);
    ~ExportVideoWidget();

    void ExportVideo();

private:
    Ui::ExportVideoWidget ui;
    MuxerVideo m_muxer_video;
};
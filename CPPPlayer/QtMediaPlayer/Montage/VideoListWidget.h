#pragma once

#include "ui_VideoListWidget.h"
#include "Montage/AnalyzeFrame.h"
#include "Common/DragListWidget.h"

#include <QDragEnterEvent>
#include <QMimeData>
#include <QTextStream>

class VideoListWidget : public DragItemWidget
{
    Q_OBJECT
public:
    VideoListWidget(AFMsg afMsg, QWidget* parent = nullptr);
    ~VideoListWidget();

    int getFileDuration() { return m_file_duration; }

    void modDuration(int64_t duration);

    void LoadKeyFrame(const std::list<AVFrame*> framelist);

    void seek(int value, std::shared_ptr<AnalyzeFrameEngine> engine);

private:
    void Init();

private:
    Ui::VideoListWidget ui;
    AFMsg m_afMsg;
    int m_timeline = 0;
    int m_frame_long = 0;     // 帧合计的长度
    int m_file_duration = 0;  // 文件总时长 单位秒
};
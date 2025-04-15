#pragma once
#include "QtMediaPlayer.h"
#include "AnalyzeManager.h"

#include <QMessageBox>

#define AUDIO_TIME_BASE 1000000
#define VIDEO_TIME_BASE 1000000

#define vAnalyzeManager AnalyzeManager::getInstance()

inline QString generateUniqueID(const QString& fileName) {
    QString timeStamp = QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz"); // ¾«È·µ½ºÁÃë
    return fileName + "_" + timeStamp;
}

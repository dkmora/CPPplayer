#pragma once
#include "ui_ExportVideoWidget.h"

class ExportVideoWidget : public QWidget
{
Q_OBJECT
public:
    ExportVideoWidget(QWidget* parent = nullptr);
    ~ExportVideoWidget();

private:
    Ui::ExportVideoWidget ui;
};
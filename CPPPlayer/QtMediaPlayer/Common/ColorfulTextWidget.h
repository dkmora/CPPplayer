#pragma once

#include <QWidget>
#include <QTimer>
#include <QLinearGradient>
#include <QPainter>

// ColorfulLabel.h
#include <QLabel>
#include <QTimer>
#include <QLinearGradient>

class ColorfulLabel : public QLabel {
    Q_OBJECT
public:
    explicit ColorfulLabel(const QString& text, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void updateGradient();

private:
    QTimer* m_timer;
    qreal m_gradientPos = 0.0;  // øÿ÷∆Ω•±‰Œª÷√
};
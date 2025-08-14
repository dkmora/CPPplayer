#include "ColorfulTextWidget.h"

ColorfulLabel::ColorfulLabel(const QString& text, QWidget* parent)
    : QLabel(text, parent) {
    // 设置字体样式
    setFont(QFont("微软雅黑", 24, QFont::Bold));

    // 初始化定时器（动态更新颜色）
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ColorfulLabel::updateGradient);
    //m_timer->start(50);  // 每50ms更新一次
}

void ColorfulLabel::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 创建水平渐变（动态变化）
    QLinearGradient gradient(0, 0, width(), 0);
    gradient.setColorAt(m_gradientPos - 0.2, Qt::red);      // 红色区域
    gradient.setColorAt(m_gradientPos, Qt::green);          // 绿色区域
    gradient.setColorAt(m_gradientPos + 0.2, Qt::blue);     // 蓝色区域

    // 绘制渐变文字
    painter.setPen(QPen(gradient, 1));
    painter.setFont(font());
    painter.drawText(rect(), Qt::AlignCenter, text());
}

void ColorfulLabel::updateGradient() {
    m_gradientPos += 0.01;  // 调整渐变速度
    if (m_gradientPos > 1.2) m_gradientPos = 0.0;
    update();  // 触发重绘
}
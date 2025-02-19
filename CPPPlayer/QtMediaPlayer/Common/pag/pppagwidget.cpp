#include "pppagwidget.h"

PPPAGWidget::PPPAGWidget(QWidget *parent)
	: QWidget(parent)
{
    //setWindowFlags(Qt::FramelessWindowHint);
    //setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    connect(&m_timer, &QTimer::timeout, this, [this]() {
        if (m_pEngine)
        {
            pp::pagEngine()->flush(m_pEngine);
        }
        raise();
    });
}

PPPAGWidget::~PPPAGWidget()
{
    stop();
    Destory();
}

void PPPAGWidget::paly(const QString& path, int msec /*= 20*/)
{
    stop();
    m_pEngine = pp::pagEngine()->initEngine(winId(), this, width(), height());
    if (m_pEngine)
    {
        pp::pagEngine()->loadFile(std::string(path.toUtf8()), m_pEngine);
        pp::pagEngine()->flush(m_pEngine);
    }
    m_timer.start(msec);
    raise();
    show();
    //activateWindow();
}

void PPPAGWidget::Resize(int width, int height)
{
    if (m_pEngine)
    {
        pp::pagEngine()->resize(width, height, m_pEngine);
    }
}

void PPPAGWidget::Destory()
{
    pp::destroypEngine();
}

void PPPAGWidget::stop()
{
    hide();
    m_timer.stop();
    if (m_pEngine)
    {
        pp::pagEngine()->deleteEngine(m_pEngine);
        m_pEngine = nullptr;
    }
}

void PPPAGWidget::OnPagPlayEnd()
{
    emit sigPagPlayEnd();
}
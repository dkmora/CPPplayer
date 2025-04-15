#pragma once
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>

#include <QDrag>
#include <QMimeData>
#include <QPainter>
#include <QTimer>
#include <QThread>
#include <QMouseEvent>
#include <QApplication>
#include <QDragMoveEvent>
#include <QPainterPath>
#include <QList>
#include <QScrollBar>
#include "gStruct.h"

class DragItemWidget : public QWidget, public QListWidgetItem {

	Q_OBJECT
public:
	DragItemWidget(QWidget* parent = nullptr);
	~DragItemWidget();

	void Init();

	//QByteArray getDropData();
	AFMsg getDropData() { return m_afMsg; };
	void setDropData(const AFMsg& afMsg);
	bool getIsDrop() { return m_isdrop; }
	void setDrop(bool isDrop) { m_isdrop = isDrop; }

	DragItemWidget* getDropButton() { return this; }
	void SetVisible(bool isShow);

private:
	virtual void paintEvent(QPaintEvent*);
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

signals:
	void sigModifyPrank(/*PrankData prankdata*/);

private:
	//QDragButton* m_btn_Drop;
	QToolButton* m_btn_delete;
	QToolButton* m_btn_modify;

	bool m_isdrop = false;

	AFMsg m_afMsg;
};

class MyTimer : public QTimer
{
	Q_OBJECT

public:
	explicit MyTimer(QObject* parent = nullptr) :
		QTimer(parent) {
		setTimerType(Qt::PreciseTimer);
	}

public slots:
	void getInterval(int value) { setInterval(value); }
	void beginTimer() { start(10); }
	void stopTimer() { stop(); }
};

class DragListWidget : public QListWidget
{
	Q_OBJECT

public:
	explicit DragListWidget(QWidget* parent = nullptr);
	~DragListWidget();

	void AddWidgetItem(DragItemWidget* widgetItem);
	void InsertWidgetItem(int insertrow, DragItemWidget* widgetItem);
	QList<AFMsg> GetItemDataList();

	bool isDraging() const { return m_isdraging; }
	int offset() const { return 19; }
	int highlightedRow() const { return m_highlightrow; }
	int dragRow() const { return m_dragrow; }
	int selectedRow() const { return m_selectedrow; }
	static QString myMimeType() { return QStringLiteral("MyListWidget/text_icon"); }

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dragLeaveEvent(QDragLeaveEvent* event) override;
	void dragMoveEvent(QDragMoveEvent* event) override;
	void dropEvent(QDropEvent* event) override;

signals:
	void sigInsertDragItem(AFMsg dragMsg);

private:
	QPoint m_startpos;
	bool m_isdraging = false;
	QRect m_rectoldhighlighted;
	QRect m_recthighlighted;
	int m_highlightrow = -1;
	int m_dragrow = -1;
	int m_selectedrow = -1;
	int m_insertrow = -1;

	const int m_scrollmargin = 70;
	int m_scrollbarvalue = 0;
	int m_scrollbarrange = 0;
	int m_timecount = 0;
	const int m_timelimit = 20;
	QThread* m_timerthread;

	const QRect targetRect(const QPoint& position) const;

private slots:
	void doAutoScroll();

public slots:
	void getScrollBarValue(int value) { m_scrollbarvalue = value; }
	void getscrollBarRange(int min, int max) { Q_UNUSED(min); m_scrollbarrange = max; }

signals:
	void exceptedValue(int value);
	void Interval(int interval);
	void beginDrag();
	void endDrag();
};

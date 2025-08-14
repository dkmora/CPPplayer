#include "DragListWidget.h"

DragItemWidget::DragItemWidget(QWidget* parent/* = nullptr*/) :
	QWidget(parent)
{
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
	setStyleSheet("background-color: transparent;");
}

DragItemWidget::~DragItemWidget() {

}

void DragItemWidget::Init() {

}

void DragItemWidget::setDropData(const AFMsg& afMsg)
{
	m_afMsg = afMsg;
}

void DragItemWidget::SetVisible(bool isShow) {
	//m_btn_Drop->setVisible(isShow);
}

void DragItemWidget::paintEvent(QPaintEvent*) {
	//QPainter Painter(this);
	//Painter.setRenderHint(QPainter::Antialiasing, true);
	//Painter.setPen(Qt::NoPen);
	//Painter.setBrush(QColor(255, 255, 255, 100));

	//QPainterPath PainterPath;
	//PainterPath.addRoundedRect(QRect(0, 0, width(), height()), 0, 0);  //Rect
	//Painter.drawPath(PainterPath);

	//// 画分界线
	//Painter.setRenderHint(QPainter::Antialiasing, true);
	//Painter.setPen(QPen(QColor("#f4f4f4"), 1));
	//QSize coresize = size();
	//Painter.drawLine(10, height(), width() - 10, height());
}

void DragItemWidget::mousePressEvent(QMouseEvent* event) {
	m_isdrop = true;
}

void DragItemWidget::mouseReleaseEvent(QMouseEvent* event) {
	m_isdrop = false;
}

DragListWidget::DragListWidget(QWidget* parent) :
	QListWidget(parent)
{
	setFrameShape(QFrame::NoFrame);
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint); // 隐藏标题栏
	setAcceptDrops(true);
	setFlow(QListView::LeftToRight);
	//setMouseTracking(true);  
	//setDragEnabled(true); 
	//setDropIndicatorShown(false); 
	//setDefaultDropAction(Qt::MoveAction);  

	//this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	//this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	this->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel); // 更精细的像素滚动
	QScrollBar* hScrollBar = this->horizontalScrollBar();
	hScrollBar->setSingleStep(2);  // 滚动更细腻
	hScrollBar->setPageStep(80);   // 一页的滚动范围

	// 使用线程是为了配合定时器，使定时器更加精确
	MyTimer* timer_scrollbar = new MyTimer;
	//scrollBarTimer->setInterval(10);
	m_timerthread = new QThread(this);
	timer_scrollbar->moveToThread(m_timerthread);
	connect(this, &DragListWidget::beginDrag, timer_scrollbar, &MyTimer::beginTimer);
	connect(this, &DragListWidget::endDrag, timer_scrollbar, &MyTimer::stopTimer);
	connect(this, &DragListWidget::Interval, timer_scrollbar, &MyTimer::getInterval);
	connect(timer_scrollbar, &MyTimer::timeout, this, &DragListWidget::doAutoScroll);
	connect(m_timerthread, &QThread::finished, timer_scrollbar, &MyTimer::deleteLater);
	m_timerthread->start();
}

DragListWidget::~DragListWidget() {
	m_timerthread->quit();
	m_timerthread->wait();
}

void DragListWidget::AddWidgetItem(DragItemWidget* widgetItem) {
	QListWidgetItem* item = new QListWidgetItem();
	item->setSizeHint(widgetItem->size());
	addItem(item);
	widgetItem->setSizeIncrement(widgetItem->size());
	setItemWidget(item, widgetItem);
}

void DragListWidget::InsertWidgetItem(int insertrow, DragItemWidget* widgetItem) {
	QListWidgetItem* pitem = new QListWidgetItem();
	pitem->setSizeHint(widgetItem->size());
	insertItem(insertrow, pitem);
	setItemWidget(pitem, widgetItem);
}

QList<AFMsg> DragListWidget::GetItemDataList()
{
	QList<AFMsg> list;
	for (int i = 0; i < this->count(); ++i) {
		DragItemWidget* _time = static_cast<DragItemWidget*>(this->itemWidget(this->item(i)));
		list.append(_time->getDropData());
	}
	return list;
}

int DragListWidget::getHorizontalScrollBar()
{
	QScrollBar* hScrollBar = this->horizontalScrollBar();
	if(hScrollBar) return hScrollBar->value();
}

void DragListWidget::setHorizontalScrollBar(int value)
{
	QScrollBar* hScrollBar = this->horizontalScrollBar();
	if(hScrollBar) hScrollBar->setValue(value);
}

//拖拽起点
void DragListWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->buttons() & Qt::LeftButton) {
		m_startpos = event->pos();
	}
}

void DragListWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if ((event->pos() - m_startpos).manhattanLength() > 5)
		return;
}

void DragListWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (event->buttons() & Qt::LeftButton) {

		// 超过规定距离才会触发拖拽，防止手滑...
		//if ((event->pos() - startPos).manhattanLength() < QApplication::startDragDistance()) 
		//	return;

		auto listmap = mapToGlobal(this->pos());
		auto posx = this->pos().x();
		auto posy = this->pos().y();

		// 鼠标点击窗口的相对坐标
		int x = event->globalPos().x() - listmap.x() + posx;
		int y = event->globalPos().y() - listmap.y() + posy;

		//qDebug() << "press x:" << x;
		//qDebug() << "press y:" << y;

		m_dragrow = row(itemAt(x, y));

		DragItemWidget* dragitem = static_cast<DragItemWidget*>(this->itemWidget(item(m_dragrow)));
		if (dragitem == nullptr)
			return;

		/*
		auto rect = dragitem->getDropButton()->geometry();
		qDebug() << "button x:" << rect.x();
		qDebug() << "button y:" << rect.y();
		qDebug() << "press x:" << x;
		qDebug() << "press y:" << y;
		int dragx = x - dragitem->getDropButton()->x();
		int dragy = y - dragitem->getDropButton()->y();
		*/

		// 判断鼠标是否在拖拽按钮的范围内
		//if (!dragitem->getDropButton()->geometry().contains(x, y - 5))
		//	return;

		auto btnpos = dragitem->getDropButton()->pos();

		if (!dragitem->getIsDrop())
			return;

		m_selectedrow = row(currentItem());
		QByteArray itemData;
		QMimeData* mimeData = new QMimeData;
		mimeData->setData(myMimeType(), itemData);

		// 拖拽缩略图
		QPixmap pixmap = dragitem->grab();
		QDrag* drag = new QDrag(this);
		drag->setMimeData(mimeData);
		//设置缩略图
		drag->setPixmap(pixmap);
		//设置鼠标在缩略图上的位置
		drag->setHotSpot(QPoint(/*btnpos.x() + */dragitem->getDropButton()->width() / 2, pixmap.height() / 2));
		dragitem->SetVisible(false);

        //qDebug() << "press x:" << btnpos.x();
        //qDebug() << "getDropButton width:" << dragitem->getDropButton()->width();

		//拖拽开始
		if (drag->exec(Qt::MoveAction) == Qt::MoveAction) {
			//if (theSelectedRow != theDragRow) {
			//	delete takeItem(theDragRow);
			//}
		}
		dragitem->SetVisible(true);
		dragitem->setDrop(false); // 拖拽结束 复位
	}
}

void DragListWidget::dragEnterEvent(QDragEnterEvent* event)
{
	DragListWidget* source = qobject_cast<DragListWidget*>(event->source());
	if (source && source == this) {
		//IsDraging(标志位)判断是否正在拖拽
		m_isdraging = true;
		emit beginDrag();  //定时器开始(检测鼠标位置到listWidget上下边缘的距离)
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}
}

//当拖拽离开QListWidget时，需要update以保证DropIndicator消失
void DragListWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
	emit endDrag();  //定时器停止
	m_highlightrow = -2;
	if (!m_recthighlighted.contains(QRect(0, 40 * m_dragrow, width(), 40))) {
		update(m_recthighlighted);
	}

	//IsDraging(标志位)判断是否正在拖拽
	m_isdraging = false;

	m_insertrow = -1;
	event->accept();
}

//拖拽移动时刷新以更新DropIndicator
void DragListWidget::dragMoveEvent(QDragMoveEvent* event)
{
	DragListWidget* source = qobject_cast<DragListWidget*>(event->source());
	if (source && source == this) {

		// 控制QscrollBar滚动速度。鼠标离边缘越近，速度越快
		if (m_timecount >= m_timelimit) {
			int posYToParent = event->pos().y(); //timer interval control
			if (posYToParent >= height() - m_scrollmargin) {
				emit Interval((height() - posYToParent + 5) / 3);
			}
			else if (posYToParent <= m_scrollmargin) {
				emit Interval((posYToParent + 5) / 3);
			}
		}

		m_rectoldhighlighted = m_recthighlighted;
		m_recthighlighted = targetRect(event->pos());

		if (/*event-> pos().y() >= offset()*/1) {
			m_highlightrow = row(itemAt(event->pos() - QPoint(0, offset())));

			if (m_rectoldhighlighted != m_recthighlighted) {
				update(m_rectoldhighlighted);  //刷新旧区域使DropIndicator消失
				update(m_recthighlighted);  //刷新新区域使DropIndicator显示
			}
			else
				update(m_recthighlighted);

			m_insertrow = row(itemAt(event->pos()));
		}
		else {
			m_highlightrow = -1;
			update(QRect(0, 0, width(), 80));  //仅刷新第一行
			m_insertrow = 0;
		}

		event->setDropAction(Qt::MoveAction);
		event->accept();
		qDebug() << "theInsertRow:" << m_insertrow;
	}
}

void DragListWidget::dropEvent(QDropEvent* event)
{
	DragListWidget* source = qobject_cast<DragListWidget*>(event->source());
	if (source && source == this) {

		m_isdraging = false;  //拖拽完成
		emit endDrag();  //定时器停止

		m_highlightrow = -2;
		update(m_recthighlighted);  //拖拽完成，刷新以使DropIndicator消失

		//如果拖拽行即选中行，则可以直接调用父类dropEvent(event)
		//if (m_selectedrow == m_dragrow) {
		//	QListWidget::dropEvent(event);
		//	return;
		//}

		if (m_dragrow == m_insertrow) {
			QListWidget::dropEvent(event);
			return;
		}

		//QByteArray itemData = event->mimeData()->data(myMimeType());
		DragItemWidget* dragitem = static_cast<DragItemWidget*>(this->itemWidget(item(m_dragrow)));
		//DragItemWidget *insertitem = static_cast<DragItemWidget *>(this->itemWidget(item(m_insertrow)));
		if (dragitem == nullptr) return;

		//auto newItem = new DragItemWidget(/*dragData*/);
		//delete takeItem(m_dragrow);
		//InsertWidgetItem(m_insertrow, newItem);

		auto dragData = dragitem->getDropData();
		delete takeItem(m_dragrow);
		dragData.insertrow = m_insertrow;
		emit sigInsertDragItem(dragData);

		if (m_insertrow > m_dragrow) {
			m_insertrow++;
		}
	}
}

//响应子线程中QTimer的timeout()信号
void DragListWidget::doAutoScroll()
{
	QPoint pos = mapFromGlobal(QCursor::pos());  //获取鼠标位置
	int posYToParent = pos.y();
	int step = 1;

	// 当timeCount > timeLimit时才会触发autoScroll
	if (posYToParent >= height() - m_scrollmargin && m_scrollbarvalue < m_scrollbarrange) {
		if (m_timecount >= m_timelimit) {
			emit exceptedValue(m_scrollbarvalue + step);  //控制QScrollBar
			if (pos.y() + m_scrollbarvalue >= offset()) {
				m_highlightrow = row(itemAt(pos - QPoint(0, offset())));

				m_rectoldhighlighted = m_recthighlighted;
				m_recthighlighted = targetRect(pos);

				if (m_rectoldhighlighted != m_recthighlighted) {
					update(m_rectoldhighlighted);
					update(m_recthighlighted);
				}
				m_insertrow = row(itemAt(pos - QPoint(0, offset()))) + 1;
			}
		}
		else {
			m_timecount++;
		}
	}
	else if (posYToParent <= m_scrollmargin && m_scrollbarvalue > 0) {
		if (m_timecount >= m_timelimit) {
			emit exceptedValue(m_scrollbarvalue - step);  //控制QScrollBar
			if (pos.y() + m_scrollbarvalue >= offset()) {
				m_highlightrow = row(itemAt(pos - QPoint(0, offset())));

				m_rectoldhighlighted = m_recthighlighted;
				m_recthighlighted = targetRect(pos);

				if (m_rectoldhighlighted != m_recthighlighted) {
					update(m_rectoldhighlighted);
					update(m_recthighlighted);
				}
				m_insertrow = row(itemAt(pos - QPoint(0, offset()))) + 1;
			}
			else if (pos.y() + m_scrollbarvalue >= 0) {
				m_highlightrow = -1;
				update(QRect(0, 0, width(), 80));
				m_insertrow = 0;
			}
			else {
				m_highlightrow = -2;
				update(QRect(0, 0, width(), 40));
				m_insertrow = -1;
			}
		}
		else {
			m_timecount++;
		}
	}
	else {
		m_timecount = 0;
	}
}

const QRect DragListWidget::targetRect(const QPoint& position) const
{
	//40是item的行高
	if (position.y() >= offset())
		return QRect(0, (position.y() - offset()) / 40 * 40, width(), 2 * 40);
	else
		return QRect(0, 0, width(), 40);
}

#include "frameLess.h"

FrameLess::FrameLess(int index, QWidget* target) :
    _target(target),
    _cursorchanged(false),
    _leftButtonPressed(false),
    _borderWidth(5),
    _index(index)
{
    _target->setAttribute(Qt::WA_Hover);
    //auto childList = _target->findChildren<QWidget*>();
    //for (auto child : childList) {
    //  child->installEventFilter(this);
    //}
    _target->installEventFilter(this);
    //_rubberband = new DragShadow();
}

FrameLess::~FrameLess()
{
    //if (_rubberband) {
    //    delete _rubberband;
    //    _rubberband = nullptr;
    //}
}

bool FrameLess::eventFilter(QObject* o, QEvent* e)
{
    auto type = e->type();
    switch (type) {
    case QEvent::MouseMove: {
        mouseMove(static_cast<QMouseEvent*>(e));
        break;
    }
    case QEvent::HoverMove: {
        mouseHover(static_cast<QHoverEvent*>(e));
        break;
    }
    case QEvent::Leave: {
        mouseLeave(e);
        break;
    }
    case QEvent::MouseButtonPress: {
        mousePress(static_cast<QMouseEvent*>(e));
        break;
    }
    case QEvent::MouseButtonRelease: {
        mouseRealese(static_cast<QMouseEvent*>(e));
        break;
    }
    default: {
        return false;
    }
    }
    //不是改变大小事件时返回子窗口事件
    if (!_mousePress.testFlag(Edge::None)) {
        return true;
    } else {
        return false;
    }
}

void FrameLess::mouseHover(QHoverEvent* e)
{
    //updateCursorShape(_target->mapToGlobal(e->pos()));
    updateCursorShape(e->pos());
}

void FrameLess::mouseLeave(QEvent* e)
{
    if (!_leftButtonPressed) {
        _target->unsetCursor();
    }
}

void FrameLess::mousePress(QMouseEvent* e)
{
    if (e->button() & Qt::LeftButton) {
        //_rubberband->setGeometry(_target->frameGeometry());
        _originRect = _target->frameGeometry();
        _leftButtonPressed = true;
        int _height = _target->minimumHeight();
        calculateCursorPosition(e->pos(), _target->frameGeometry(), _mousePress);
    }
}

void FrameLess::mouseRealese(QMouseEvent* e)
{
    if (e->button() & Qt::LeftButton) {
        //_target->move(0, 0);
        _leftButtonPressed = false;
        //if (_rubberband && _rubberband->isVisible()) {
        //    _rubberband->hide();
        //    _target->setGeometry(_rubberband->geometry());
        //}
    }
}

void FrameLess::mouseMove(QMouseEvent* e)
{
    if (_leftButtonPressed) {
        int _offset = 0;
        int _x = 0;
        if (_index != 0) {
            _offset = _target->width();
        }

        if (!_mousePress.testFlag(Edge::None) /*&& !_target->ismaximized()*/) { //_target prevent resizing if it is maximized
            QRect newRect = _originRect;
            switch (_mousePress) {
            case Edge::Left:
                newRect.setLeft(e->pos().x());
                if (newRect.width() < _target->minimumWidth()) {
                    newRect.setLeft(_originRect.right() - _target->minimumWidth());
                    //newRect.setWidth(_target->minimumWidth());
                }
                break;
            case Edge::Right:
                _x = e->pos().x();
                qDebug() << "_x = " << _x;
                newRect.setRight(_offset + e->pos().x());
                if (newRect.width() < _target->minimumWidth()) {
                    //newRect.setRight(_originRect.left() + _target->minimumWidth());
                }
                break;
            default:
                return;
            }
            //qDebug() << "ffwidth = " << newRect.width();

            int _width = newRect.width();
            if (_index != 0)
            {
                _width = _x;
            }
            else
            {
                //_width = _x;
            }

            emit sigFrameLessWidth(_width, _index);
            //_target->setGeometry(newRect);
            //_target->setMaximumSize(newRect.width(), newRect.height());
            _target->show();
        }
    } else {
        updateCursorShape(e->pos());
    }
}

void FrameLess::updateCursorShape(const QPoint& pos)
{
    if (_target->isFullScreen() /*|| _target->isMaximized()*/) {
        if (_cursorchanged) {
            _target->unsetCursor();
        }
        return;
    }
    if (!_leftButtonPressed) {
        auto framerect = _target->frameGeometry();
        //framerect.setX(0);
        calculateCursorPosition(pos, framerect, _mouseMove);
        _cursorchanged = true;
        if (_mouseMove.testFlag(Edge::Top) || _mouseMove.testFlag(Edge::Bottom)) {
            //_target->setCursor(Qt::SizeVerCursor);
        } else if (_mouseMove.testFlag(Edge::Left) || _mouseMove.testFlag(Edge::Right)) {
            _target->setCursor(Qt::SizeHorCursor);
        } else if (_mouseMove.testFlag(Edge::TopLeft) || _mouseMove.testFlag(Edge::BottomRight)) {
            //_target->setCursor(Qt::SizeFDiagCursor);
        } else if (_mouseMove.testFlag(Edge::TopRight) || _mouseMove.testFlag(Edge::BottomLeft)) {
           // _target->setCursor(Qt::SizeBDiagCursor);
        } else if (_cursorchanged) {
            _target->unsetCursor();
            _cursorchanged = false;
        }
    }
}

void FrameLess::calculateCursorPosition(const QPoint& pos, const QRect& framerect, Edges& _edge)
{
    int _offset = 0;
    int _x = 0;
    if (_index != 0)
    {
        _offset = framerect.width();
        _x = 0;
    }
    else
    {
        _x = framerect.x();
    }

    bool onLeft = pos.x() >= framerect.x() - _borderWidth + _offset && pos.x() <= framerect.x() + _borderWidth;
    bool onRight = pos.x() >= _x + framerect.width() - _borderWidth && pos.x() <= _x + framerect.width();

    if (onLeft) {
        _edge = Left;
    } else if (onRight) {
        _edge = Right;
    } else {
        _edge = None;
    }
}
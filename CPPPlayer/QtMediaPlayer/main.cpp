#define SDL_MAIN_HANDLED
#include "QtMediaPlayer.h"
#include <QtWidgets/QApplication>
#include <objbase.h>

void EnableDrag(QMainWindow& w) {
    ChangeWindowMessageFilter(WM_DROPFILES, 1);

    w.winId() << w.effectiveWinId();
    ChangeWindowMessageFilterEx((HWND)w.effectiveWinId(), WM_DROPFILES, MSGFLT_ALLOW, NULL);
    ChangeWindowMessageFilterEx((HWND)w.effectiveWinId(), WM_COPYDATA, MSGFLT_ALLOW, NULL);
    ChangeWindowMessageFilterEx((HWND)w.effectiveWinId(), 0x0049, MSGFLT_ALLOW, NULL);

    DragAcceptFiles((HWND)w.effectiveWinId(), true);
    RevokeDragDrop((HWND)w.winId());
}

int main(int argc, char *argv[])
{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    QApplication a(argc, argv);
    QtMediaPlayer w;
    EnableDrag(w);
    g_MediaPlayer = &w;
    w.show();
    return a.exec();
}

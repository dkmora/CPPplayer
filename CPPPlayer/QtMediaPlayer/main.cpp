#define SDL_MAIN_HANDLED
#include "QtMediaPlayer.h"
#include <QtWidgets/QApplication>
#include <objbase.h>

int main(int argc, char *argv[])
{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    QApplication a(argc, argv);
    QtMediaPlayer w;
    w.show();
    return a.exec();
}

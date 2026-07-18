#include <QApplication>
#include "serverui.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    ServerUi w;
    w.show();

    return a.exec();
}
#include <QApplication>
#include "serverui.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/BoocClubServer.jpg"));

    ServerUi w;
    w.show();

    return a.exec();
}
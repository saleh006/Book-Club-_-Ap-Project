#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "servermanager.h"
#include "databasemanager.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (!DatabaseManager::instance().initialize("bookclub.db")) {
        return -1;
    }

    ServerManager *server = new ServerManager();
    if (!server->startServer(1234)) {
        qCritical() << "سرور نتوانست روی پورت 1234 روشن شود. احتمالا پورت اشغال است!";
    }

    return a.exec();
}
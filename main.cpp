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
        qCritical() << "cant start server in port 1234 !!!!";
    }

    return a.exec();
}
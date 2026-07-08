#include <QCoreApplication>
#include <QDebug>
#include "servermanager.h"
#include "databasemanager.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    if (!DatabaseManager::instance().initialize("bookclub.db")) {
        qCritical() << "Failed to initialize database on Server!";
        return -1;
    }

    ServerManager *server = new ServerManager();
    if (!server->startServer(1234)) {
        qCritical() << "Server could not start on port 1234!";
        return -1;
    }

    qDebug() << "Server is successfully running and listening on port 1234...";
    return a.exec();
}
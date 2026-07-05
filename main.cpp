#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QQuickWidget>
#include <QUrl>
#include "servermanager.h"
#include "databasemanager.h"

int main(int argc, char *argv[])
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QApplication a(argc, argv);
    if (!DatabaseManager::instance().initialize("bookclub.db")) {
        return -1;
    }
    ServerManager *server = new ServerManager();
    if (!server->startServer(1234)) {
        qCritical() << "سرور نتوانست روی پورت 1234 روشن شود. احتمالا پورت اشغال است!";
    }
    QQuickWidget *view = new QQuickWidget;
    view->setSource(QUrl("qrc:/BookClubAuth/src/auth/firstPage.qml"));
    view->setResizeMode(QQuickWidget::SizeRootObjectToView);
    view->setWindowTitle("Book Club - Welcome");
    view->resize(800,500);
    view->show();
    return a.exec();
}
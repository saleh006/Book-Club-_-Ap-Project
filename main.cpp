#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QQuickWidget>
#include <QUrl>
#include <QStackedWidget>
#include <QQuickItem>
#include "loginwindow.h"
#include "servermanager.h"
#include "databasemanager.h"
#include "signupwindow.h"

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
    QQuickWidget *firstPageWidget = new QQuickWidget;
    firstPageWidget->setSource(QUrl("qrc:/BookClubAuth/src/auth/firstPage.qml"));
    firstPageWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    firstPageWidget->setWindowTitle("Book Club - Welcome");
    firstPageWidget->resize(800, 500);
    LoginWindow *loginWin = new LoginWindow();
    SignupWindow *signupWin = new SignupWindow();

    QQuickItem *qmlRoot = firstPageWidget->rootObject();
    if (qmlRoot) {
        QObject::connect(qmlRoot, SIGNAL(loginRequested()), loginWin, SLOT(show()));
        QObject::connect(qmlRoot, SIGNAL(loginRequested()), firstPageWidget, SLOT(hide()));

        QObject::connect(qmlRoot, SIGNAL(signupRequested()), signupWin, SLOT(show()));
        QObject::connect(qmlRoot, SIGNAL(signupRequested()), firstPageWidget, SLOT(hide()));

        QObject::connect(qmlRoot, SIGNAL(exitRequested()), &a, SLOT(quit()));
    }

    QObject::connect(loginWin, &LoginWindow::switchToSignUpRequested, [&]() {
        loginWin->hide();
        signupWin->show();
    });

    QObject::connect(signupWin, &SignupWindow::switchToLoginRequested, [&]() {
        signupWin->hide();
        loginWin->show();
    });

    QObject::connect(loginWin, &LoginWindow::backToMainRequested, [&]() {
        loginWin->hide();
        firstPageWidget->show();
    });

    QObject::connect(signupWin, &SignupWindow::backToMainRequested, [&]() {
        signupWin->hide();
        firstPageWidget->show();
    });

    QObject::connect(loginWin, &LoginWindow::loginSuccessful, [&](const QString &username) {
        const QString role = loginWin->loggedInRole();
        qDebug() << "Logged in as" << username << "role:" << role;
        loginWin->clearFields();
        loginWin->hide();
        // پنجره های پنل کاربر ها رو باید اینجا باز کنیم
    });

    firstPageWidget->show();
    return a.exec();
}
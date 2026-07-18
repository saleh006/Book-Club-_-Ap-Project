#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QQuickWidget>
#include <QUrl>
#include <QStackedWidget>
#include <QQuickItem>
#include "loginwindow.h"
#include "signupwindow.h"
#include "recoverywindow.h"
#include "src/adminPanel/adminpanel.h"
#include "src/publisherPanel/publisherpanel.h"

int main(int argc, char *argv[])
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QApplication a(argc, argv);

    QQuickWidget *firstPageWidget = new QQuickWidget;
    firstPageWidget->setSource(QUrl("qrc:/BookClubAuth/src/auth/firstPage.qml"));
    firstPageWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    firstPageWidget->setWindowTitle("Book Club - Welcome");
    firstPageWidget->resize(800, 500);

    LoginWindow *loginWin = new LoginWindow();
    SignupWindow *signupWin = new SignupWindow();
    RecoveryWindow *recoveryWin = new RecoveryWindow();
    AdminPanel *adminWin = new AdminPanel();

    QObject *qmlRoot = firstPageWidget->rootObject();
    if(qmlRoot){
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

    QObject::connect(loginWin, &LoginWindow::useRecoveryAnswer, [&]() {
        loginWin->hide();
        recoveryWin->show();
    });

    QObject::connect(recoveryWin, &RecoveryWindow::switchToLoginRequested, [&]() {
        recoveryWin->hide();
        loginWin->show();
    });

    QObject::connect(recoveryWin, &RecoveryWindow::passwordResetSuccessful, [&]() {
        recoveryWin->clearFields();
        recoveryWin->hide();
        loginWin->show();
        loginWin->showSuccessMessage("Password changed successfully! Please log in.");
    });

    QObject::connect(signupWin, &SignupWindow::signupSuccessful, [&](const QString &username) {
        signupWin->clearFields();
        signupWin->hide();
        firstPageWidget->show();
        QMetaObject::invokeMethod(qmlRoot, "showNotification", Q_ARG(QVariant, "Registration successful :)"));
    });

    QObject::connect(adminWin,&AdminPanel::logoutRequested,[&](){
        adminWin->hide();
        loginWin->show();
    });

    QObject::connect(loginWin, &LoginWindow::loginSuccessful, [&](const QString &username) {
        const QString role = loginWin->loggedInRole();
        const int userId = loginWin->loggedInUserId();
        const QString fullName = loginWin->loggedInFullName();
        qDebug() << "Logged in as" << username << "role:" << role;
        loginWin->clearFields();
        loginWin->hide();
        if (role == "admin") {
            loginWin->hide();
            adminWin->show();
        }
        if(role == "publisher"){
            loginWin->hide();
            PublisherPanel *publesherWin = new PublisherPanel(userId,fullName,username);
            publesherWin->show();
            QObject::connect(publesherWin,&PublisherPanel::logoutRequested,[=]{
                publesherWin->deleteLater();
                firstPageWidget->show();
            });
        }
        else {
            qDebug() << "Redirecting to regular user dashboard...";
        }
    });

    firstPageWidget->show();
    return a.exec();
}
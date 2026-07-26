#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QQuickWidget>
#include <QUrl>
#include <QStackedWidget>
#include <QQuickItem>
#include <functional>
#include <QAction>
#include <QEvent>
#include <QMessageBox>

#include "loginwindow.h"
#include "signupwindow.h"
#include "recoverywindow.h"
#include "src/adminPanel/adminpanel.h"
#include "src/publisherPanel/publisherpanel.h"
#include "src/userPanel/userpanel.h"
#include "styledmessagebox.h"

class MessageBoxStyleFixer : public QObject {
    QString m_style;
public:
    MessageBoxStyleFixer(const QString &style, QObject *parent = nullptr)
        : QObject(parent), m_style(style) {}

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {

        if (event->type() == QEvent::Show) {
            if (QMessageBox *msgBox = qobject_cast<QMessageBox*>(obj)) {
                if (!msgBox->property("styleFixed").toBool()) {
                    msgBox->setStyleSheet(m_style);
                    msgBox->setProperty("styleFixed", true);
                }
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

int main(int argc, char *argv[])
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QApplication a(argc, argv);

    MessageBoxStyleFixer *fixer = new MessageBoxStyleFixer(kGlobalMessageBoxStyle, &a);
    a.installEventFilter(fixer);

    QQuickWidget *firstPageWidget = new QQuickWidget;
    firstPageWidget->setSource(QUrl("qrc:/BookClubAuth/src/auth/firstPage.qml"));
    firstPageWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    firstPageWidget->setWindowTitle("Book Club - Welcome");
    firstPageWidget->resize(800, 500);

    QObject *qmlRoot = firstPageWidget->rootObject();

    std::function<void(QString)> openLogin;
    std::function<void()> openSignup;
    std::function<void()> openRecovery;
    std::function<void()> openAdmin;

    openLogin = [&](QString msg) {
        LoginWindow *loginWin = new LoginWindow();

        QObject::connect(loginWin, &LoginWindow::switchToSignUpRequested, [&, loginWin]() {
            loginWin->deleteLater();
            openSignup();
        });

        QObject::connect(loginWin, &LoginWindow::backToMainRequested, [&, loginWin]() {
            loginWin->deleteLater();
            firstPageWidget->show();
        });

        QObject::connect(loginWin, &LoginWindow::useRecoveryAnswer, [&, loginWin]() {
            loginWin->deleteLater();
            openRecovery();
        });

        QObject::connect(loginWin, &LoginWindow::loginSuccessful, [&, loginWin](const QString &username) {
            const QString role = loginWin->loggedInRole();
            const int userId = loginWin->loggedInUserId();
            const QString fullName = loginWin->loggedInFullName();
            qDebug() << "Logged in as" << username << "role:" << role;

            loginWin->deleteLater();

            if (role == "admin") {
                openAdmin();
            }
            else if(role == "publisher"){
                PublisherPanel *publisherWin = new PublisherPanel(userId, fullName, username);
                publisherWin->show();
                QObject::connect(publisherWin, &PublisherPanel::logoutRequested, [=](){
                    publisherWin->deleteLater();
                    firstPageWidget->show();
                });
            }
            else if(role == "user"){
                UserPanel *userWin = new UserPanel(userId, fullName, username);
                userWin->show();
                QObject::connect(userWin, &UserPanel::logoutRequested, [=](){
                    userWin->deleteLater();
                    firstPageWidget->show();
                });
            }
        });

        loginWin->show();

        if (!msg.isEmpty()) {
            loginWin->showSuccessMessage(msg);
        }
    };

    openSignup = [&]() {
        SignupWindow *signupWin = new SignupWindow();

        QObject::connect(signupWin, &SignupWindow::switchToLoginRequested, [&, signupWin]() {
            signupWin->deleteLater();
            openLogin("");
        });

        QObject::connect(signupWin, &SignupWindow::backToMainRequested, [&, signupWin]() {
            signupWin->deleteLater();
            firstPageWidget->show();
        });

        QObject::connect(signupWin, &SignupWindow::signupSuccessful, [&, signupWin](const QString &username) {
            signupWin->deleteLater();
            firstPageWidget->show();
            if(qmlRoot) {
                QMetaObject::invokeMethod(qmlRoot, "showNotification", Q_ARG(QVariant, "Registration successful :)"));
            }
        });

        signupWin->show();
    };

    openRecovery = [&]() {
        RecoveryWindow *recoveryWin = new RecoveryWindow();

        QObject::connect(recoveryWin, &RecoveryWindow::switchToLoginRequested, [&, recoveryWin]() {
            recoveryWin->deleteLater();
            openLogin("");
        });

        QObject::connect(recoveryWin, &RecoveryWindow::passwordResetSuccessful, [&, recoveryWin]() {
            recoveryWin->deleteLater();
            openLogin("Password changed successfully!");
        });

        recoveryWin->show();
    };

    openAdmin = [&]() {
        AdminPanel *adminWin = new AdminPanel();

        QObject::connect(adminWin, &AdminPanel::logoutRequested, [&, adminWin](){
            adminWin->deleteLater();
            firstPageWidget->show();
        });

        adminWin->show();
    };

    if(qmlRoot){
        QAction *loginBridge = new QAction(firstPageWidget);
        QAction *signupBridge = new QAction(firstPageWidget);

        QObject::connect(qmlRoot, SIGNAL(loginRequested()), loginBridge, SLOT(trigger()));
        QObject::connect(qmlRoot, SIGNAL(signupRequested()), signupBridge, SLOT(trigger()));

        QObject::connect(loginBridge, &QAction::triggered, [&](){openLogin("");});
        QObject::connect(signupBridge, &QAction::triggered, openSignup);

        QObject::connect(qmlRoot, SIGNAL(loginRequested()), firstPageWidget, SLOT(hide()));
        QObject::connect(qmlRoot, SIGNAL(signupRequested()), firstPageWidget, SLOT(hide()));
        QObject::connect(qmlRoot, SIGNAL(exitRequested()), &a, SLOT(quit()));
    }

    firstPageWidget->show();
    return a.exec();
}
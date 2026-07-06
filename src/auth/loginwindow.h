#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTcpSocket>

class QLineEdit;
class QLabel;
class QPushButton;


class LoginWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LoginWindow(QWidget *parent = nullptr);
        void clearFields();
        QString loggedInRole() const;
    signals:
        void loginSuccessful(const QString &username);
        void switchToSignUpRequested();
        void useRecoveryAnswer();
        void backToMainRequested();
    private slots:
        void handleLoginClicked();
        void onReadyRead();
        void onSocketError(QAbstractSocket::SocketError error);
private :
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QPushButton *m_loginButton;
    QLabel *m_statusLabel;
    QString m_loggedInRole;
    QTcpSocket *m_socket;

};

#endif // LOGINWINDOW_H

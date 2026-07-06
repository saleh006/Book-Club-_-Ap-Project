#ifndef SIGNUPWINDOW_H
#define SIGNUPWINDOW_H

#include <QWidget>
#include <QString>
#include <QTcpSocket>

class QLineEdit;
class QLabel;
class QPushButton;
class QComboBox;

class SignupWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SignupWindow(QWidget *parent = nullptr);
//     void clearFields();

// signals:
//     void signupSuccessful(const QString &username);
//     void switchToLoginRequested();

// private slots:
//     void handleSignupClicked();
//     void onReadyRead();
//     void onSocketError(QAbstractSocket::SocketError error);

private:
    QLineEdit *m_fullNameEdit;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_confirmPasswordEdit;
    QLineEdit *m_recoveryAnswerEdit;
    QComboBox *m_accountTypeCombo;
    QPushButton *m_signupButton;
    QLabel *m_statusLabel;
    QTcpSocket *m_socket;
};

#endif // SIGNUPWINDOW_H
#ifndef RECOVERYWINDOW_H
#define RECOVERYWINDOW_H

#include <QWidget>
#include <QString>
#include <QTcpSocket>

class QLineEdit;
class QLabel;
class QPushButton;

class RecoveryWindow : public QWidget
{
    Q_OBJECT

public:
    explicit RecoveryWindow(QWidget *parent = nullptr);
    void clearFields();

signals:
    void passwordResetSuccessful();
    void switchToLoginRequested();

private slots:
    void handleResetClicked();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);

private:
    QLineEdit *m_usernameEdit;
    QLineEdit *m_recoveryAnswerEdit;
    QLineEdit *m_newPasswordEdit;
    QLineEdit *m_confirmPasswordEdit;
    QPushButton *m_resetButton;
    QLabel *m_statusLabel;
    QTcpSocket *m_socket;
};

#endif // RECOVERYWINDOW_H
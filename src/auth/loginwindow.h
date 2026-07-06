#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QString>

class QLineEdit;
class QLabel;
class QPushButton;


class LoginWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LoginWindow(QWidget *parent = nullptr);
    //     void clearFields();
    //     QString loggedInRole() const;
    // signals:
    //     void loginSuccesseful();
    //     void switchToSignUpRequested();
    //     void useRecoveryAnswer();
    // private slots:
    //     void handleLoginClicked();
private :
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QPushButton *m_loginButton;
    QLabel *m_statusLabel;
    QString m_loggedInRole;

};

#endif // LOGINWINDOW_H

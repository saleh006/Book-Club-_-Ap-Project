#ifndef EDITPROFILEDIALOG_H
#define EDITPROFILEDIALOG_H

#include <QDialog>
#include <QLineEdit>

class EditProfileDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EditProfileDialog(const QString &username, const QString &fullName,
                               const QString &email, QWidget *parent = nullptr);
    QString username() const { return m_usernameEdit->text().trimmed(); }
    QString fullName() const      { return m_fullNameEdit->text().trimmed(); }
    QString email() const         { return m_emailEdit->text().trimmed(); }
    bool    wantsPasswordChange() const { return !m_newPassEdit->text().isEmpty(); }
    QString oldPassword() const   { return m_oldPassEdit->text(); }
    QString newPassword() const   { return m_newPassEdit->text(); }

private:
    void accept() override;   // validates before closing

    QLineEdit *m_fullNameEdit;
    QLineEdit *m_emailEdit;
    QLineEdit *m_oldPassEdit;
    QLineEdit *m_newPassEdit;
    QLineEdit *m_confirmPassEdit;
    QLineEdit *m_usernameEdit;
};

#endif // EDITPROFILEDIALOG_H

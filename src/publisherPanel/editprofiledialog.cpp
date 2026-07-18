#include "editprofiledialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QMessageBox>


EditProfileDialog::EditProfileDialog(const QString &username, const QString &fullName,
                                     const QString &email, QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Edit Profile");
    setFixedWidth(380);
    setStyleSheet(
        "QDialog { background-color: #120E14; }"
        "QLabel { color: #9A8FA0; font-size: 12px; border: none; background: transparent; }"
        "QLineEdit { background-color: #060508; border: 1px solid #1F1724; border-radius: 6px;"
        "  padding: 8px; color: #EAEAEA; font-size: 13px; }"
        "QLineEdit:focus { border: 1px solid #7C3E66; }"
        );

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(10);

    auto addField = [&](const QString &label, bool password = false) {
        layout->addWidget(new QLabel(label, this));
        auto *edit = new QLineEdit(this);
        if (password) edit->setEchoMode(QLineEdit::Password);
        layout->addWidget(edit);
        return edit;
    };

    auto *header = new QLabel("✏️  Profile Information", this);
    header->setStyleSheet("color: #EAEAEA; font-size: 14px; font-weight: bold;");
    layout->addWidget(header);

    m_usernameEdit = addField("Username");
    m_usernameEdit->setText(username);

    m_fullNameEdit = addField("Full Name");
    m_fullNameEdit->setText(fullName);

    m_emailEdit = addField("Email");
    m_emailEdit->setText(email);


    // separator
    auto *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("border: none; border-top: 1px solid #1F1724;");
    layout->addSpacing(6);
    layout->addWidget(line);
    layout->addSpacing(6);

    auto *passHeader = new QLabel("🔒  Change Password (optional)", this);
    passHeader->setStyleSheet("color: #EAEAEA; font-size: 14px; font-weight: bold;");
    layout->addWidget(passHeader);

    m_oldPassEdit     = addField("Current Password", true);
    m_newPassEdit     = addField("New Password", true);
    m_confirmPassEdit = addField("Repeat New Password", true);

    // buttons
    auto *btnRow = new QHBoxLayout;
    auto *cancelBtn = new QPushButton("Cancel", this);
    auto *saveBtn   = new QPushButton("Save Changes", this);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    saveBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet(
        "QPushButton { background: transparent; border: 1px solid #1F1724; border-radius: 6px;"
        "  padding: 8px 16px; color: #9A8FA0; }"
        "QPushButton:hover { color: #EAEAEA; border-color: #9A8FA0; }");
    saveBtn->setStyleSheet(
        "QPushButton { background-color: #7C3E66; border: none; border-radius: 6px;"
        "  padding: 8px 16px; color: white; font-weight: bold; }"
        "QPushButton:hover { background-color: #B06B96; }");
    btnRow->addStretch();
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(saveBtn);
    layout->addSpacing(8);
    layout->addLayout(btnRow);

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn,   &QPushButton::clicked, this, &EditProfileDialog::accept);
}

void EditProfileDialog::accept()
{
    if (!username().isEmpty() && username().contains(' ')) {
        QMessageBox::warning(this, "Invalid input", "Username cannot contain spaces.");
        return;
    }
    if (!email().isEmpty() && !email().contains('@')) {
        QMessageBox::warning(this, "Invalid input", "Please enter a valid email address.");
        return;
    }

    const bool anyPassFieldFilled = !m_oldPassEdit->text().isEmpty()
                                    || !m_newPassEdit->text().isEmpty()
                                    || !m_confirmPassEdit->text().isEmpty();
    if (anyPassFieldFilled) {
        if (m_oldPassEdit->text().isEmpty()) {
            QMessageBox::warning(this, "Invalid input", "Enter your current password to change it.");
            return;
        }
        if (m_newPassEdit->text().length() < 6) {
            QMessageBox::warning(this, "Invalid input", "New password must be at least 6 characters.");
            return;
        }
        if (m_newPassEdit->text() != m_confirmPassEdit->text()) {
            QMessageBox::warning(this, "Invalid input", "New passwords do not match.");
            return;
        }
    }
    QDialog::accept();
}

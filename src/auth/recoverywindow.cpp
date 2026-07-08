#include "recoverywindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QJsonObject>
#include <QJsonDocument>

RecoveryWindow::RecoveryWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("BookClub - Recover Password");
    setFixedSize(800, 500);

    QWidget *leftPanel = new QWidget(this);
    leftPanel->setObjectName("rightPanel");

    QLabel *titleLabel = new QLabel("RESET PASSWORD", leftPanel);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);

    QLabel *subtitleLabel = new QLabel("Answer your recovery question to continue.", leftPanel);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);

    m_usernameEdit = new QLineEdit(leftPanel);
    m_usernameEdit->setPlaceholderText("Username");

    m_recoveryAnswerEdit = new QLineEdit(leftPanel);
    m_recoveryAnswerEdit->setPlaceholderText("Recovery Answer");

    m_newPasswordEdit = new QLineEdit(leftPanel);
    m_newPasswordEdit->setPlaceholderText("New Password");
    m_newPasswordEdit->setEchoMode(QLineEdit::Password);

    m_confirmPasswordEdit = new QLineEdit(leftPanel);
    m_confirmPasswordEdit->setPlaceholderText("Confirm New Password");
    m_confirmPasswordEdit->setEchoMode(QLineEdit::Password);

    m_statusLabel = new QLabel(leftPanel);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setVisible(false);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);

    m_resetButton = new QPushButton("Reset Password", leftPanel);
    m_resetButton->setObjectName("loginButton");

    QLabel *backToLoginLabel = new QLabel(leftPanel);
    backToLoginLabel->setObjectName("signupLabel");
    backToLoginLabel->setText("Remembered your password? <a href=\"login\" style=\"color:#7C3E66; text-decoration:none;\">Login</a>");
    backToLoginLabel->setAlignment(Qt::AlignCenter);
    backToLoginLabel->setOpenExternalLinks(false);

    QWidget *formWidget = new QWidget(leftPanel);
    formWidget->setFixedWidth(300);

    QVBoxLayout *formLayout = new QVBoxLayout(formWidget);
    formLayout->addWidget(m_usernameEdit);
    formLayout->addWidget(m_recoveryAnswerEdit);
    formLayout->addWidget(m_newPasswordEdit);
    formLayout->addWidget(m_confirmPasswordEdit);
    formLayout->addWidget(m_statusLabel);
    formLayout->addWidget(m_resetButton);


    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(50, 10, 50, 20);
    leftLayout->setSpacing(10);

    leftLayout->addSpacing(10);
    leftLayout->addWidget(titleLabel, 0, Qt::AlignHCenter);
    leftLayout->addWidget(subtitleLabel, 0, Qt::AlignHCenter);
    leftLayout->addSpacing(14);
    leftLayout->addWidget(formWidget, 0, Qt::AlignHCenter);
    leftLayout->addSpacing(6);
    leftLayout->addWidget(backToLoginLabel);



    QWidget *rightPanel = new QWidget(this);
    rightPanel->setObjectName("leftPanel");

    QLabel *imageLabel = new QLabel(rightPanel);
    QPixmap pixmap(":/resetpass.png");
    imageLabel->setPixmap(pixmap.scaled(300, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    imageLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(imageLabel);

    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(leftPanel);
    rootLayout->addWidget(rightPanel);

    setStyleSheet(R"(
        QWidget {
            background-color: #060508;
            color: #EAEAEA;
            font-family: "Segoe UI";
        }
        #leftPanel { background-color: #120E14; }
        #titleLabel { font-size: 22px; font-weight: bold; color: #FFFFFF; }
        #subtitleLabel { font-size: 12px; color: #9A8FA0; }
        QLineEdit {
            background-color: #1F1724;
            border: 1px solid #5F2E4F;
            border-radius: 8px;
            padding: 8px 12px;
            font-size: 13px;
            color: #EAEAEA;
        }
        QLineEdit:focus { border: 1px solid #7C3E66; }
        #statusLabel { color: #E06C8C; font-size: 12px; }
        #loginButton {
            background-color: #7C3E66;
            border: none;
            border-radius: 8px;
            padding: 10px;
            font-size: 14px;
            font-weight: bold;
            color: white;
        }
        #loginButton:hover { background-color: #5F2E4F; }
        #signupLabel { color: #9A8FA0; font-size: 12px; }
    )");

    // m_socket = new QTcpSocket(this);
    // connect(m_socket, &QTcpSocket::readyRead, this, &RecoveryWindow::onReadyRead);
    // connect(m_socket, &QTcpSocket::errorOccurred, this, &RecoveryWindow::onSocketError);
    // connect(m_resetButton, &QPushButton::clicked, this, &RecoveryWindow::handleResetClicked);
    // connect(backToLoginLabel, &QLabel::linkActivated, this, [this](const QString &) {
    //     emit switchToLoginRequested();
    // });
}
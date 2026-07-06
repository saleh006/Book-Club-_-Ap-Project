#include "signupwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QJsonObject>
#include <QJsonDocument>

SignupWindow::SignupWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("BookClub - Sign Up");
    setFixedSize(800, 500);
    QWidget *leftPanel = new QWidget(this);
    leftPanel->setObjectName("rightPanel");

    QLabel *titleLabel = new QLabel("CREATE ACCOUNT", leftPanel);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);

    QLabel *subtitleLabel = new QLabel("Join the club, start reading.", leftPanel);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);

    m_fullNameEdit = new QLineEdit(leftPanel);
    m_fullNameEdit->setPlaceholderText("Full Name");

    m_usernameEdit = new QLineEdit(leftPanel);
    m_usernameEdit->setPlaceholderText("Username");

    m_passwordEdit = new QLineEdit(leftPanel);
    m_passwordEdit->setPlaceholderText("Password");
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    m_confirmPasswordEdit = new QLineEdit(leftPanel);
    m_confirmPasswordEdit->setPlaceholderText("Confirm Password");
    m_confirmPasswordEdit->setEchoMode(QLineEdit::Password);

    m_recoveryAnswerEdit = new QLineEdit(leftPanel);
    m_recoveryAnswerEdit->setPlaceholderText("Recovery Answer (e.g. favorite book)");

    m_accountTypeCombo = new QComboBox(leftPanel);
    m_accountTypeCombo->setObjectName("accountTypeCombo");
    m_accountTypeCombo->addItem("Reader", "user");
    m_accountTypeCombo->addItem("Publisher", "publisher");

    m_statusLabel = new QLabel(leftPanel);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setVisible(false);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);

    m_signupButton = new QPushButton("Sign Up", leftPanel);
    m_signupButton->setObjectName("loginButton");

    QLabel *loginLabel = new QLabel(leftPanel);
    loginLabel->setObjectName("signupLabel");
    loginLabel->setText("Already have an account? <a href=\"login\" style=\"color:#7C3E66; text-decoration:none;\">Login</a>");
    loginLabel->setAlignment(Qt::AlignCenter);
    loginLabel->setOpenExternalLinks(false);

    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(50, 20, 50, 20);
    leftLayout->setSpacing(10);

    leftLayout->addWidget(titleLabel, 0, Qt::AlignHCenter);
    leftLayout->addWidget(subtitleLabel, 0, Qt::AlignHCenter);
    leftLayout->addSpacing(14);
    leftLayout->addWidget(m_fullNameEdit);
    leftLayout->addWidget(m_usernameEdit);
    leftLayout->addWidget(m_passwordEdit);
    leftLayout->addWidget(m_confirmPasswordEdit);
    leftLayout->addWidget(m_recoveryAnswerEdit);
    leftLayout->addWidget(m_accountTypeCombo);
    leftLayout->addWidget(m_statusLabel);
    leftLayout->addSpacing(6);
    leftLayout->addWidget(m_signupButton);
    leftLayout->addWidget(loginLabel);

    QWidget *rightPanel = new QWidget(this);
    rightPanel->setObjectName("leftPanel"); // reuse same dark panel style
    rightPanel->setFixedWidth(350);

    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(leftPanel);
    rootLayout->addWidget(rightPanel);

    QLabel *imageLabel = new QLabel(rightPanel);
    QPixmap pixmap(":/bookclublogo.png");
    imageLabel->setPixmap(pixmap.scaled(300, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    imageLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(imageLabel);

    QPushButton *backBtn = new QPushButton("Back to Main", this);  //     این رو درستش کنننننننننننن

    setStyleSheet(R"(
        QWidget {
            background-color: #060508;
            color: #EAEAEA;
            font-family: "Segoe UI";
        }

        #leftPanel {
            background-color: #120E14;
        }

        #titleLabel {
            font-size: 22px;
            font-weight: bold;
            color: #FFFFFF;
        }

        #subtitleLabel {
            font-size: 12px;
            color: #9A8FA0;
        }

        QLineEdit, QComboBox {
            background-color: #1F1724;
            border: 1px solid #5F2E4F;
            border-radius: 8px;
            padding: 8px 12px;
            font-size: 13px;
            color: #EAEAEA;
        }

        QLineEdit:focus, QComboBox:focus {
            border: 1px solid #7C3E66;
        }

        QComboBox::drop-down {
            border: none;
        }

        #statusLabel {
            color: #E06C8C;
            font-size: 12px;
        }

        #loginButton {
            background-color: #7C3E66;
            border: none;
            border-radius: 8px;
            padding: 10px;
            font-size: 14px;
            font-weight: bold;
            color: white;
        }

        #loginButton:hover {
            background-color: #5F2E4F;
        }

        #signupLabel {
            color: #9A8FA0;
            font-size: 12px;
        }
    )");

    // m_socket = new QTcpSocket(this);
    // connect(m_socket, &QTcpSocket::readyRead, this, &SignupWindow::onReadyRead);
    // connect(m_socket, &QTcpSocket::errorOccurred, this, &SignupWindow::onSocketError);

    // connect(m_signupButton, &QPushButton::clicked, this, &SignupWindow::handleSignupClicked);

    connect(loginLabel, &QLabel::linkActivated, this, [this](const QString &) {
        emit switchToLoginRequested();
    });

    connect(backBtn, &QPushButton::clicked, this, [this]() {
        emit backToMainRequested();
    });
}

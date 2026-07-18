#include "loginwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QColor>
#include <QPixmap>
#include <QDebug>
#include <QQuickWidget>

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("BookClub - Login");
    setFixedSize(800, 500);

    QWidget *leftPanel = new QWidget(this);
    leftPanel->setObjectName("leftPanel");
    leftPanel->setFixedWidth(450);

    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    QQuickWidget *qmlStickerWidget = new QQuickWidget(leftPanel);

    qmlStickerWidget->setSource(QUrl::fromLocalFile(":/StickerView.qml"));
    qmlStickerWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    leftLayout->addWidget(qmlStickerWidget);

    QWidget *rightPanel = new QWidget(this);
    rightPanel->setObjectName("rightPanel");

    QLabel *titleLabel = new QLabel("BOOK CLUB", rightPanel);
    titleLabel->setObjectName("titleLabel");

    QLabel *subtitleLabel = new QLabel("Read . Sip . Enjoy", rightPanel);
    subtitleLabel->setObjectName("subtitleLabel");

    m_usernameEdit = new QLineEdit(rightPanel);
    m_usernameEdit->setObjectName("usernameEdit");
    m_usernameEdit->setPlaceholderText("Username");

    m_passwordEdit = new QLineEdit(rightPanel);
    m_passwordEdit->setObjectName("passwordEdit");
    m_passwordEdit->setPlaceholderText("Password");
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    m_statusLabel = new QLabel(rightPanel);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setVisible(false);

    m_loginButton = new QPushButton("Login", rightPanel);
    m_loginButton->setObjectName("loginButton");

    QLabel *signupLabel = new QLabel(rightPanel);
    signupLabel->setObjectName("signupLabel");
    signupLabel->setText("Don't have an account? <a href=\"signup\" style=\"color:#7C3E66; text-decoration:none;\">Sign up</a>");
    signupLabel->setAlignment(Qt::AlignCenter);
    signupLabel->setOpenExternalLinks(false);

    QLabel *recoveryLabel = new QLabel(rightPanel);
    recoveryLabel->setObjectName("recoveryLabel");
    recoveryLabel->setText("Forgot password? <a href=\"recovery\" style=\"color:#7C3E66; text-decoration:none;\">Use recovery answer</a>");
    recoveryLabel->setAlignment(Qt::AlignCenter);
    recoveryLabel->setOpenExternalLinks(false);

    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(50, 0, 50, 0);
    rightLayout->setSpacing(14);

    titleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setAlignment(Qt::AlignCenter);

    rightLayout->addStretch();
    rightLayout->addWidget(titleLabel);
    rightLayout->addWidget(subtitleLabel);
    rightLayout->addSpacing(30);
    rightLayout->addWidget(m_usernameEdit);
    rightLayout->addWidget(m_passwordEdit);
    rightLayout->addWidget(m_statusLabel);
    rightLayout->addSpacing(6);
    rightLayout->addWidget(m_loginButton);
    rightLayout->addSpacing(16);
    rightLayout->addWidget(recoveryLabel);
    rightLayout->addWidget(signupLabel);
    rightLayout->addStretch();

    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(leftPanel);
    rootLayout->addWidget(rightPanel);


    QPushButton *backBtn = new QPushButton("← Back", this);
    backBtn->setObjectName("backButton");
    backBtn->move(696 , 4);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setFixedSize(95, 32);

    //qss
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
            font-size: 26px;
            font-weight: bold;
            color: #FFFFFF;
        }

        #subtitleLabel {
            font-size: 13px;
            color: #9A8FA0;
        }

        QLineEdit {
            background-color: #1F1724;
            border: 1px solid #5F2E4F;
            border-radius: 8px;
            padding: 10px 12px;
            font-size: 14px;
            color: #EAEAEA;
        }

        QLineEdit:focus {
            border: 1px solid #7C3E66;
        }

        #statusLabel {
            color: #E06C8C;
            font-size: 12px;
        }

        #loginButton {
            background-color: #7C3E66;
            border: none;
            border-radius: 8px;
            padding: 12px;
            font-size: 14px;
            font-weight: bold;
            color: white;
        }

        #loginButton:hover {
            background-color: #5F2E4F;
        }

        #backButton {
            background-color: transparent;
            border: 1px solid #7C3E66;
            border-radius: 15px;
            color: #D9C2D1;
            font-size: 11px;
            font-weight: 600;
            padding: 4px 10px;
        }

        #backButton:hover {
             background-color: rgba(124, 62, 102, 60);
            color: white;
            border: 1px solid #B06B96;
        }

        #backButton:pressed {
            background-color: rgba(124, 62, 102, 120);
        }

        #secondaryButton {
            background-color: #1F1724;
            border: 1px solid #5F2E4F;
            border-radius: 8px;
            padding: 12px;
            font-size: 14px;
            font-weight: bold;
            color: #EAEAEA;
        }

        #secondaryButton:hover {
            background-color: #120E14;
        }
        #signupLabel, #recoveryLabel {
            color: #9A8FA0;
            font-size: 12px;
        }
    )");
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::readyRead, this, &LoginWindow::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &LoginWindow::onSocketError);

    connect(m_loginButton, &QPushButton::clicked, this, &LoginWindow::handleLoginClicked);

    connect(signupLabel, &QLabel::linkActivated, this, [this](const QString &) {
        emit switchToSignUpRequested();
    });

    connect(recoveryLabel, &QLabel::linkActivated, this, [this](const QString &) {
        emit useRecoveryAnswer();
    });

    connect(backBtn, &QPushButton::clicked, this, [this]() {
        emit backToMainRequested();
    });
}

void LoginWindow::clearFields()
{
    m_usernameEdit->clear();
    m_passwordEdit->clear();
    m_statusLabel->setStyleSheet("color: #E06C8C; font-size: 12px;");
    m_statusLabel->clear();
    m_statusLabel->setVisible(false);
}

void LoginWindow::showSuccessMessage(const QString &message)
{
    m_statusLabel->setStyleSheet("color: #2ECC71; font-size: 12px;");
    m_statusLabel->setText(message);
    m_statusLabel->setVisible(true);
}

QString LoginWindow::loggedInRole() const
{
    return m_loggedInRole;
}

void LoginWindow::handleLoginClicked()
{
    const QString username = m_usernameEdit->text().trimmed();
    const QString password = m_passwordEdit->text();
    if (username.isEmpty() || password.isEmpty()) {
        m_statusLabel->setText("Please enter both username and password.");
        m_statusLabel->setVisible(true);
        return;
    }
    m_statusLabel->setVisible(false);
    m_loginButton->setEnabled(false);
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        m_socket->connectToHost("127.0.0.1", 1234);
        if (!m_socket->waitForConnected(3000)) {
            m_statusLabel->setText("Could not connect to server.");
            m_statusLabel->setVisible(true);
            m_loginButton->setEnabled(true);
            return;
        }
    }
    QJsonObject requestObj;
    requestObj["action"] = "login";
    requestObj["username"] = username;
    requestObj["password"] = password;
    QJsonDocument doc(requestObj);
    m_socket->write(doc.toJson(QJsonDocument::Compact) + "\n");
}

void LoginWindow::onReadyRead()
{
    const QByteArray data = m_socket->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);

    m_loginButton->setEnabled(true);

    if (!doc.isObject()) {
        m_statusLabel->setText("Invalid server response.");
        m_statusLabel->setVisible(true);
        return;
    }

    const QJsonObject responseObj = doc.object();
    const QString status = responseObj["status"].toString();

    if (status == "success") {
        m_loggedInRole = responseObj["role"].toString();
        m_loggedInUserId = responseObj["userId"].toInt();       // NEW
        m_loggedInFullName = responseObj["fullName"].toString(); // NEW
        emit loginSuccessful(m_usernameEdit->text().trimmed());
    }
    else {
        m_statusLabel->setText(responseObj["message"].toString());
        m_statusLabel->setVisible(true);
    }
}

void LoginWindow::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    m_loginButton->setEnabled(true);
    m_statusLabel->setText("Connection error: " + m_socket->errorString());
    m_statusLabel->setVisible(true);
}

int LoginWindow::loggedInUserId() const
{
    return m_loggedInUserId;
}

QString LoginWindow::loggedInFullName() const
{
    return m_loggedInFullName;
}
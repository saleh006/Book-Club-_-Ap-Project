#include "loginwindow.h"
#include "databasemanager.h"
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
}
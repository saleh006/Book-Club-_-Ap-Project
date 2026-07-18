#include "serverui.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QDebug>
#include <QStyle>
#include <QFont>

ServerUi::ServerUi(QWidget *parent)
    : QWidget(parent), m_server(new ServerManager()), m_isRunning(false)
{
    setupUI();

    if (!DatabaseManager::instance().initialize("bookclub.db")) {
        m_statusLabel->setText("Database Error: Could not initialize!");
        m_statusLabel->setStyleSheet("color: #E06C8C; font-size: 14px; font-weight: bold;");
    }
}

ServerUi::~ServerUi()
{
    delete m_server;
}

void ServerUi::setupUI()
{
    setWindowTitle("BookClub - Server Manager");
    setFixedSize(440, 320);

    setStyleSheet(R"(
        QWidget {
            background-color: #14121A;
            color: #EDE7DD;
            font-family: "Segoe UI";
        }
        #titleLabel {
            font-family: "Georgia", "Times New Roman", serif;
            font-size: 25px;
            font-weight: bold;
            letter-spacing: 1px;
            color: #FFFFFF;
        }
        #subtitleLabel {
            font-size: 12px;
            color: #8B84A0;
            letter-spacing: 2px;
        }
        #statusCard {
            background-color: transparent;
            border: 1px solid #2A2732;
            border-radius: 10px;
        }
        #toggleButton {
            background-color: #7C3E66;
            border: none;
            border-radius: 8px;
            padding: 12px;
            font-size: 15px;
            font-weight: bold;
            letter-spacing: 1px;
            color: #F5EFE6;
        }
        #toggleButton:hover {
            background-color: #9A5480;
        }
        #toggleButton:pressed {
            background-color: #5F2E4F;
        }
        #toggleButton[state="running"] {
            background-color: #2A2732;
            border: 2px solid #E0708C;
            color: #E0708C;
        }
        #toggleButton[state="running"]:hover {
            background-color: #33303D;
        }
    )");

    QHBoxLayout *outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    QWidget *content = new QWidget(this);
    outerLayout->addWidget(content, 1);

    QVBoxLayout *mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(36, 34, 36, 34);
    mainLayout->setSpacing(18);

    QLabel *titleLabel = new QLabel("Server Control", content);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);

    QLabel *subtitleLabel = new QLabel("BOOKCLUB BACKEND SYSTEM", content);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);

    QFrame *statusCard = new QFrame(content);
    statusCard->setObjectName("statusCard");
    QVBoxLayout *statusLayout = new QVBoxLayout(statusCard);
    statusLayout->setContentsMargins(16, 14, 16, 14);

    m_statusLabel = new QLabel("○ OFFLINE", statusCard);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: #E0708C; font-family: 'Consolas', 'Courier New', monospace; font-size: 14px; font-weight: bold; letter-spacing: 1px; border: none;");
    statusLayout->addWidget(m_statusLabel);

    m_toggleButton = new QPushButton("START SERVER", content);
    m_toggleButton->setObjectName("toggleButton");
    m_toggleButton->setCursor(Qt::PointingHandCursor);
    m_toggleButton->setFixedSize(320, 50);

    connect(m_toggleButton, &QPushButton::clicked, this, &ServerUi::toggleServer);

    mainLayout->addStretch();
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(statusCard);
    mainLayout->addWidget(m_toggleButton, 0, Qt::AlignHCenter);
    mainLayout->addStretch();
}

void ServerUi::toggleServer()
{
    if (!m_isRunning) {
        if (m_server->startServer(1234)) {
            m_isRunning = true;
            m_statusLabel->setText("● ONLINE  —  PORT 1234");
            m_statusLabel->setStyleSheet("color: #5FBF8A; font-family: 'Consolas', 'Courier New', monospace; font-size: 14px; font-weight: bold; letter-spacing: 1px; border: none;");

            m_toggleButton->setText("STOP SERVER");
            m_toggleButton->setProperty("state", "running");
            m_toggleButton->style()->unpolish(m_toggleButton);
            m_toggleButton->style()->polish(m_toggleButton);
        } else {
            m_statusLabel->setText("● ERROR  —  PORT 1234 UNAVAILABLE");
            m_statusLabel->setStyleSheet("color: #E0708C; font-family: 'Consolas', 'Courier New', monospace; font-size: 14px; font-weight: bold; letter-spacing: 1px; border: none;");
        }
    }
    else {
        m_server->stopServer();

        m_isRunning = false;
        m_statusLabel->setText("○ OFFLINE");
        m_statusLabel->setStyleSheet("color: #8B84A0; font-family: 'Consolas', 'Courier New', monospace; font-size: 14px; font-weight: bold; letter-spacing: 1px; border: none;");

        m_toggleButton->setText("START SERVER");
        m_toggleButton->setProperty("state", "stopped");
        m_toggleButton->style()->unpolish(m_toggleButton);
        m_toggleButton->style()->polish(m_toggleButton);
    }
}
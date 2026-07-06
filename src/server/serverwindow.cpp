#include "serverwindow.h"
#include <QGroupBox>
#include <QDateTime>

ServerWindow::ServerWindow(ServerManager *server, QWidget *parent )
    : QMainWindow(parent), m_serverManager(server) // ساخت نمونه از سرور شما[cite: 2, 3]
{
    setupUi();
    connect(m_serverManager, &ServerManager::serverLogEvent, this, &ServerWindow::onNewLogReceived);
    connect(m_serverManager, &ServerManager::clientCountChanged, this, &ServerWindow::onClientCountUpdated);
    m_statusLabel->setText("🟢 Server Status: ACTIVE (Port 1234)");
    m_statusLabel->setStyleSheet("color: #2ecc71; font-weight: bold; font-size: 14px;");
    onNewLogReceived("System: Server successfully booted and listening...");
}

ServerWindow::~ServerWindow()
{}

void ServerWindow::setupUi()
{
    this->setWindowTitle("BookClub - Advanced Server Dashboard");
    this->resize(800, 600);
    this->setStyleSheet("background-color: #1e1e24; color: #ffffff; font-family: 'Segoe UI', Arial;");

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    QHBoxLayout *statsLayout = new QHBoxLayout();

    m_statusLabel = new QLabel("⚪ Server Status: Checking...", this);
    m_clientCountLabel = new QLabel("👥 Active Clients: 0", this);
    m_cpuLabel = new QLabel("💻 CPU Usage: -- %", this);
    m_ramLabel = new QLabel("🧠 RAM Usage: -- %", this);

    m_statusLabel->setStyleSheet("background-color: #2a2a35; border: 1px solid #3a3a4c; border-radius: 6px; padding: 10px; font-size: 13px;");
    m_clientCountLabel->setStyleSheet("background-color: #2a2a35; border: 1px solid #3a3a4c; border-radius: 6px; padding: 10px; font-size: 13px; color: #3498db; font-weight: bold;");
    m_cpuLabel->setStyleSheet("background-color: #2a2a35; border: 1px solid #3a3a4c; border-radius: 6px; padding: 10px; font-size: 13px; color: #f1c40f; font-weight: bold;");
    m_ramLabel->setStyleSheet("background-color: #2a2a35; border: 1px solid #3a3a4c; border-radius: 6px; padding: 10px; font-size: 13px; color: #9b59b6; font-weight: bold;");

    statsLayout->addWidget(m_statusLabel);
    statsLayout->addWidget(m_clientCountLabel);
    statsLayout->addWidget(m_cpuLabel);
    statsLayout->addWidget(m_ramLabel);
    mainLayout->addLayout(statsLayout);

    QGroupBox *logGroup = new QGroupBox("Live Activity Logs (Real-time)", this);
    logGroup->setStyleSheet("QGroupBox { font-weight: bold; color: #e0e0e0; border: 1px solid #3a3a4c; border-radius: 8px; margin-top: 10px; padding-top: 15px; }");
    QVBoxLayout *logGroupLayout = new QVBoxLayout(logGroup);

    m_logDisplay = new QTextEdit(this);
    m_logDisplay->setReadOnly(true);
    m_logDisplay->setStyleSheet("background-color: #0f0f12; color: #a3be8c; border: none; font-family: 'Consolas', monospace; font-size: 12px;");

    logGroupLayout->addWidget(m_logDisplay);
    mainLayout->addWidget(logGroup);

    this->setCentralWidget(centralWidget);
}

void ServerWindow::onNewLogReceived(const QString &message)
{
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    m_logDisplay->append(QString("[%1] %2").arg(currentTime, message));
}
void ServerWindow::onClientCountUpdated(int count)
{
    m_clientCountLabel->setText(QString("👥 Active Clients: %1").arg(count));
}
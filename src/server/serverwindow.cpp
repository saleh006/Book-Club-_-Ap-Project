#include "serverwindow.h"
#include <QGroupBox>
#include <QDateTime>

ServerWindow::ServerWindow(ServerManager *server, QWidget *parent )
    : QMainWindow(parent), m_serverManager(server)
{
    setupUi();
    connect(m_serverManager, &ServerManager::serverLogEvent, this, &ServerWindow::onNewLogReceived);
    connect(m_serverManager, &ServerManager::clientCountChanged, this, &ServerWindow::onClientCountUpdated);
    m_statusLabel->setText("🟢 Server Status: ACTIVE (Port 1234)");
    m_statusLabel->setStyleSheet("color: #2ecc71; font-weight: bold; font-size: 14px;");
    onNewLogReceived("System: Server successfully booted and listening...");
#ifdef Q_OS_WIN
    GetSystemTimes(&m_preIdleTime, &m_preKernelTime, &m_preUserTime);
#endif
    m_sysTimer = new QTimer(this);
    connect(m_sysTimer, &QTimer::timeout, this, &ServerWindow::updateSystemUsage);
    m_sysTimer->start(1000);
    onNewLogReceived("System: GUI Dashboard connected. Resource monitoring started.");
}

#ifdef Q_OS_WIN
double ServerWindow::getCpuUsage()
{
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) return 0.0;

    auto ft2u64 = [](const FILETIME &ft) {
        return (static_cast<quint64>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    };

    quint64 idle = ft2u64(idleTime) - ft2u64(m_preIdleTime);
    quint64 kernel = ft2u64(kernelTime) - ft2u64(m_preKernelTime);
    quint64 user = ft2u64(userTime) - ft2u64(m_preUserTime);
    quint64 system = kernel + user;

    m_preIdleTime = idleTime;
    m_preKernelTime = kernelTime;
    m_preUserTime = userTime;

    if (system == 0) return 0.0;
    return static_cast<double>(system - idle) * 100.0 / system;
}
#endif

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
void ServerWindow::updateSystemUsage()
{
#ifdef Q_OS_WIN
    double cpuPercent = getCpuUsage();
    m_cpuLabel->setText(QString("💻 CPU Usage: %1 %").arg(cpuPercent, 0, 'f', 1));
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        int ramPercent = memInfo.dwMemoryLoad;
        m_ramLabel->setText(QString("🧠 RAM Usage: %1 %").arg(ramPercent));
    }
#else
    m_cpuLabel->setText("💻 CPU Usage: N/A");
    m_ramLabel->setText("🧠 RAM Usage: N/A");
#endif
}
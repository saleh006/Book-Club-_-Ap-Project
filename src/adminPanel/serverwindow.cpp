#include "serverwindow.h"
#include <QGroupBox>
#include <QDateTime>
#include <QTabWidget>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

ServerWindow::ServerWindow( QWidget *parent )
    : QWidget(parent)
{
    setupUi();

    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &ServerWindow::onConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &ServerWindow::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ServerWindow::onDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &ServerWindow::onSocketError);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(2000);
    m_reconnectTimer->setInterval(2000); // retry every 2s
    connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (m_socket->state() == QAbstractSocket::UnconnectedState) {
            m_socket->connectToHost("127.0.0.1", 1234);
        }
    });
    m_reconnectTimer->start();

    m_socket->connectToHost("127.0.0.1", 1234);

    m_statusLabel->setText("🟡 Connecting to Server...");
    m_statusLabel->setStyleSheet("color: #f39c12; font-weight: bold; font-size: 14px;");

    m_sysTimer = new QTimer(this);
    connect(m_sysTimer, &QTimer::timeout, this, &ServerWindow::updateSystemUsage);
    m_sysTimer->start(1000);
}

ServerWindow::~ServerWindow()
{
    if (m_socket && m_socket->isOpen()) {
        m_socket->disconnectFromHost();
    }
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

void ServerWindow::setupUi()
{
    this->setStyleSheet("color: #EAEAEA; font-family: 'Segoe UI', Arial;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15,15,15,15);
    mainLayout->setSpacing(12);

    QGridLayout *statsGrid = new QGridLayout();
    statsGrid->setSpacing(8);

    QWidget *statusCard = createStatCard("", m_statusLabel, "⚪ Server Status: Checking...", "#f39c12");
    QWidget *clientCard = createStatCard(":/icons/users-solid.png", m_clientCountLabel, "Active Clients: 0", "#3498db");
    QWidget *cpuCard    = createStatCard(":/icons/computer-solid.png", m_cpuLabel, "CPU Usage: -- %", "#f1c40f");
    QWidget *ramCard    = createStatCard(":/icons/memory-solid.png", m_ramLabel, "RAM Usage: -- %", "#9b59b6");

    statsGrid->addWidget(statusCard, 0, 0);
    statsGrid->addWidget(clientCard, 0, 1);
    statsGrid->addWidget(cpuCard,    1, 0);
    statsGrid->addWidget(ramCard,    1, 1);

    statsGrid->setColumnStretch(0, 1);
    statsGrid->setColumnStretch(1, 1);

    mainLayout->addLayout(statsGrid);



    QGroupBox *logGroup = new QGroupBox("Live Activity Logs (Real-time)", this);

    logGroup->setStyleSheet(
        "QGroupBox {"
        "font-weight:bold;"
        "color:#e0e0e0;"
        "border:1px solid #3a3a4c;"
        "border-radius:8px;"
        "margin-top:5px;"
        "padding-top:15px;"
        "}"
        );


    QVBoxLayout *logGroupLayout = new QVBoxLayout(logGroup);
    logGroupLayout->setContentsMargins(10,10,10,10);


    m_logDisplay = new QTextEdit(this);
    m_logDisplay->setReadOnly(true);

    m_logDisplay->setStyleSheet(
        "background-color:#0f0f12;"
        "color:#a3be8c;"
        "border:none;"
        "font-family:'Consolas', monospace;"
        "font-size:13px;"
        );


    logGroupLayout->addWidget(m_logDisplay);

    mainLayout->addWidget(logGroup);
}

void ServerWindow::onNewLogReceived(const QString &message)
{
    QString currentTime = QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss]");
    QString formattedMessage = QString("<span style='color: #7f8c8d;'>%1</span> %2").arg(currentTime, message);
    m_logDisplay->append(formattedMessage);
    m_logDisplay->moveCursor(QTextCursor::End);
}
void ServerWindow::onClientCountUpdated(int count)
{
    m_clientCountLabel->setText(QString("Active Clients: %1").arg(count));
}
void ServerWindow::updateSystemUsage()
{
#ifdef Q_OS_WIN
    double cpuPercent = getCpuUsage();
    m_cpuLabel->setText(QString("CPU Usage: %1 %").arg(cpuPercent, 0, 'f', 1));
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        int ramPercent = memInfo.dwMemoryLoad;
        m_ramLabel->setText(QString("RAM Usage: %1 %").arg(ramPercent));
    }
#else
    m_cpuLabel->setText("💻 CPU Usage: N/A");
    m_ramLabel->setText("🧠 RAM Usage: N/A");
#endif
}

void ServerWindow::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;

    QJsonObject response = doc.object();
    QString type = response["type"].toString();

    if (type == "log") {
        onNewLogReceived(response["message"].toString());
    }
    else if (type == "client_count") {
        onClientCountUpdated(response["count"].toInt());
    }
}

void ServerWindow::onConnected(){
    m_statusLabel->setText("🟢 Server Status: ACTIVE ");

    m_statusLabel->setStyleSheet("color: #2ecc71; font-weight: bold; font-size: 14px; border: none; background: transparent;");
    onNewLogReceived("System: Connected to the remote server core successfully.");
    QJsonObject req;
    req["action"] = "admin_subscribe";
    m_socket->write(QJsonDocument(req).toJson(QJsonDocument::Compact) + "\n");
}

void ServerWindow::onDisconnected()
{
    m_statusLabel->setText("🔴 Server Status: OFFLINE");

    m_statusLabel->setStyleSheet("color: #e74c3c; font-weight: bold; font-size: 14px; border: none; background: transparent;");
    onNewLogReceived("System: Lost connection to the server core.");
    m_clientCountLabel->setText("Active Clients: 0");
}

void ServerWindow::onSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    m_statusLabel->setText("🔴 Server Status: UNREACHABLE");
    QString cardStyle =
        "QLabel { background-color: #120E14; border: 1px solid #1F1724; border-radius: 6px; "
        "padding: 8px; font-size: 12px; font-weight: bold; color: #e74c3c; }";
    m_statusLabel->setStyleSheet(cardStyle);
}

QWidget* ServerWindow::createStatCard(const QString &iconPath, QLabel *&textLabel,
                                      const QString &initialText, const QString &textColor)
{
    QFrame *card = new QFrame(this);
    card->setStyleSheet(QString(
        "QFrame {"
        "background-color: #120E14;"
        "border: 1px solid #1F1724;"
        "border-radius: 6px;"
        "}"
        ));

    QHBoxLayout *layout = new QHBoxLayout(card);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(8);
    layout->setAlignment(Qt::AlignCenter);

    if (!iconPath.isEmpty()) {
        QLabel *icon = new QLabel(card);
        icon->setFixedSize(20, 20);
        icon->setPixmap(QPixmap(iconPath)
                            .scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        layout->addWidget(icon);
    }

    textLabel = new QLabel(initialText, card);
    textLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 14px; border: none; background: transparent;")
                                 .arg(textColor));
    layout->addWidget(textLabel);

    return card;
}
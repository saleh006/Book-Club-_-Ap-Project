#include "serverwindow.h"
#include <QGroupBox>
#include <QDateTime>
#include <QTabWidget>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

ServerWindow::ServerWindow( QWidget *parent )
    : QMainWindow(parent)
{
    setupUi();

    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &ServerWindow::onConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &ServerWindow::onReadyRead);
    m_socket->connectToHost("127.0.0.1",1234);

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
    this->setWindowTitle("BookClub - Advanced Server Dashboard");
    this->resize(800, 600);
    this->setStyleSheet("background-color: #1e1e24; color: #ffffff; font-family: 'Segoe UI', Arial;");

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->setStyleSheet("QTabBar::tab { background: #2a2a35; color: white; padding: 8px 20px; border: 1px solid #3a3a4c; } "
                             "QTabBar::tab:selected { background: #3498db; font-weight: bold; } "
                             "QTabWidget::panel { border: 1px solid #3a3a4c; background: #1e1e24; }");
    QWidget *monitorTab = new QWidget();
    QVBoxLayout *monitorLayout = new QVBoxLayout(monitorTab);
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
    monitorLayout->addLayout(statsLayout);

    QGroupBox *logGroup = new QGroupBox("Live Activity Logs (Real-time)", this);
    logGroup->setStyleSheet("QGroupBox { font-weight: bold; color: #e0e0e0; border: 1px solid #3a3a4c; border-radius: 8px; margin-top: 10px; padding-top: 15px; }");
    QVBoxLayout *logGroupLayout = new QVBoxLayout(logGroup);

    m_logDisplay = new QTextEdit(this);
    m_logDisplay->setReadOnly(true);
    m_logDisplay->setStyleSheet("background-color: #0f0f12; color: #a3be8c; border: none; font-family: 'Consolas', monospace; font-size: 12px;");

    logGroupLayout->addWidget(m_logDisplay);
    monitorLayout->addWidget(logGroup);


    QWidget *dbTab = new QWidget();
    usersTable = new QTableWidget(this);
    usersTable->setColumnCount(4);
    usersTable->setHorizontalHeaderLabels({"ID", "Username", "Full Name", "Role"});
    usersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    usersTable->setStyleSheet("background-color: #0f0f12; color: white; gridline-color: #3a3a4c; QHeaderView::section { background-color: #2a2a35; color: white; }");
    usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    booksTable = new QTableWidget(this);
    booksTable->setColumnCount(4);
    booksTable->setHorizontalHeaderLabels({"ID", "Title", "Author", "Price"});
    booksTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    booksTable->setStyleSheet("background-color: #0f0f12; color: white; gridline-color: #3a3a4c; QHeaderView::section { background-color: #2a2a35; color: white; }");
    booksTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QVBoxLayout *dbLayout = new QVBoxLayout(dbTab);
    QLabel *usersTitle = new QLabel("👥 All Registered Users:", this);
    usersTitle->setStyleSheet("font-weight: bold; font-size: 14px; color: #3498db; margin-top: 5px;");
    dbLayout->addWidget(usersTitle);
    dbLayout->addWidget(usersTable);
    QLabel *booksTitle = new QLabel("📚 Library Books:", this);
    booksTitle->setStyleSheet("font-weight: bold; font-size: 14px; color: #2ecc71; margin-top: 5px;");
    dbLayout->addWidget(booksTitle);
    dbLayout->addWidget(booksTable);

    tabWidget->addTab(monitorTab,"📈 Live Monitor");
    tabWidget->addTab(dbTab,"🗄️ Database Live Sync");
    mainLayout->addWidget(tabWidget);
    this->setCentralWidget(centralWidget);
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
void ServerWindow::loadUsersFromDatabase()
{
    usersTable->setRowCount(0);
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    QJsonObject request;
    request["action"] = "admin_get_users";
    QJsonDocument doc(request);
    m_socket->write(doc.toJson(QJsonDocument::Compact) + "\n");
}
void ServerWindow::loadBooksFromDatabase()
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;

    QJsonObject request;
    request["action"] = "admin_get_books";

    QJsonDocument doc(request);
    m_socket->write(doc.toJson(QJsonDocument::Compact) + "\n");
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
    else if (type == "users_list") {
        usersTable->setRowCount(0);
        QJsonArray usersArray = response["users"].toArray();
        for (int i = 0; i < usersArray.size(); ++i) {
            QJsonObject u = usersArray[i].toObject();
            usersTable->insertRow(i);
            usersTable->setItem(i, 0, new QTableWidgetItem(u["username"].toString()));
            usersTable->setItem(i, 1, new QTableWidgetItem(u["name"].toString()));
            usersTable->setItem(i, 2, new QTableWidgetItem(u["email"].toString()));
            usersTable->setItem(i, 3, new QTableWidgetItem(u["role"].toString()));

            usersTable->item(i, 0)->setTextAlignment(Qt::AlignCenter);
            usersTable->item(i, 1)->setTextAlignment(Qt::AlignCenter);
            usersTable->item(i, 2)->setTextAlignment(Qt::AlignCenter);
            usersTable->item(i, 3)->setTextAlignment(Qt::AlignCenter);
        }
    }
    else if (type == "books_list") {
        booksTable->setRowCount(0);
        QJsonArray booksArray = response["books"].toArray();
        for (int i = 0; i < booksArray.size(); ++i) {
            QJsonObject b = booksArray[i].toObject();
            booksTable->insertRow(i);
            booksTable->setItem(i, 0, new QTableWidgetItem(QString::number(b["id"].toInt())));
            booksTable->setItem(i, 1, new QTableWidgetItem(b["title"].toString()));
            booksTable->setItem(i, 2, new QTableWidgetItem(b["author"].toString()));
            booksTable->setItem(i, 3, new QTableWidgetItem(QString::number(b["price"].toDouble()) + " $"));

            booksTable->item(i, 0)->setTextAlignment(Qt::AlignCenter);
            booksTable->item(i, 1)->setTextAlignment(Qt::AlignCenter);
            booksTable->item(i, 2)->setTextAlignment(Qt::AlignCenter);
            booksTable->item(i, 3)->setTextAlignment(Qt::AlignCenter);
        }
    }
}

void ServerWindow::onConnected(){
    m_statusLabel->setText("🟢 Server Status: ACTIVE (Connected via Network)");
    m_statusLabel->setStyleSheet("color: #2ecc71; font-weight: bold; font-size: 14px;");
    onNewLogReceived("System: Connected to the remote server core successfully.");
    loadUsersFromDatabase();
    loadBooksFromDatabase();
}
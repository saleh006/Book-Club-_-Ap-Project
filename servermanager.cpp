#include "servermanager.h"

ClientHandler::ClientHandler(qintptr socketDescriptor, QObject *parent)
    : QThread(parent), m_socketDescriptor(socketDescriptor), m_socket(nullptr)
{
    this->start();
}

ClientHandler::~ClientHandler()
{
    if (this->isRunning()) {
        this->quit();
        this->wait();
    }
}

void ClientHandler::run()
{
    qDebug() << "ترد جدید برای کلاینت ساخته شد. آی‌دی ترد:" << QThread::currentThreadId();

    m_socket = new QTcpSocket();

    if (!m_socket->setSocketDescriptor(m_socketDescriptor)) {
        qWarning() << "خطا در تنظیم شناسه سوکت کلاینت!";
        m_socket->deleteLater();
        return;
    }

    connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected);

    exec();
}

void ClientHandler::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    QString message = QString::fromUtf8(data);
    qDebug() << "پیام دریافت شده از کلاینت:" << message;

    m_socket->write("Hello from Multi-threaded Server!\n");
    m_socket->flush();
}

void ClientHandler::onDisconnected()
{
    qDebug() << "کلاینت ارتباطش را قطع کرد. پاکسازی حافظه...";
    m_socket->close();
    m_socket->deleteLater();
    this->quit();
    this->deleteLater();
}

ServerManager::ServerManager(QObject *parent)
    : QTcpServer(parent)
{}

bool ServerManager::startServer(int port)
{
    QSqlQuery walQuery;
    if (!walQuery.exec("PRAGMA journal_mode=WAL;")) {
        qWarning() << "خطا در فعالسازی حالت WAL دیتابیس:" << walQuery.lastError().text();
    } else {
        qDebug() << "دیتابیس دگرگون شد: حالت WAL برای مدیریت هم‌زمانی تردها فعال است.";
    }

    if (!this->listen(QHostAddress::Any, port)) {
        qWarning() << "سرور روی پورت" << port << "روشن نشد:" << this->errorString();
        return false;
    }

    qDebug() << "سرور باشگاه کتاب با موفقیت روی پورت" << port << "شروع به کار کرد...";
    return true;
}

void ServerManager::incomingConnection(qintptr socketDescriptor)
{
    new ClientHandler(socketDescriptor, this);
}
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include "databasemanager.h"
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
    QByteArray rawData = m_socket->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(rawData);

    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        m_socket->write("Error: Invalid JSON format\n");
        m_socket->flush();
        return;
    }

    QJsonObject requestObj = jsonDoc.object();
    QString action = requestObj["action"].toString();
    QJsonObject responseObj;

    if (action == "login") {
        QString username = requestObj["username"].toString();
        QString password = requestObj["password"].toString();

        QString errorMsg;
        User loggedInUser;
        bool isSuccess = DatabaseManager::instance().authenticateUser(username, password, errorMsg, &loggedInUser);

        if (isSuccess) {
            responseObj["status"] = "success";
            responseObj["message"] = "ورود موفقیت‌آمیز بود.";
            responseObj["role"] = loggedInUser.role;
            responseObj["fullName"] = loggedInUser.fullName;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "register") {
        QString username = requestObj["username"].toString();
        QString password = requestObj["password"].toString();
        QString fullName = requestObj["fullName"].toString();
        QString email = requestObj["email"].toString();
        QString recoveryAnswer = requestObj["recoveryAnswer"].toString();
        QString role = requestObj["role"].toString();
        QString errorMsg;

        bool isSuccess = DatabaseManager::instance().registerUser(
            username, password, fullName, email, recoveryAnswer, errorMsg, role
            );
        if (isSuccess) {
            responseObj["status"] = "success";
            responseObj["message"] = "ثبت‌نام با موفقیت انجام شد. اکنون می‌توانید وارد شوید.";
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else {
        responseObj["status"] = "error";
        responseObj["message"] = "عملیات نامعتبر است.";
    }

    QJsonDocument responseDoc(responseObj);
    m_socket->write(responseDoc.toJson(QJsonDocument::Compact));
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
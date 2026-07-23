
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include "clienthandler.h"
#include "servermanager.h"

ServerManager::ServerManager(QObject *parent)
    : QTcpServer(parent)
{
    connect(this, &ServerManager::serverLogEvent, this, [this](const QString &msg) {
        QJsonObject obj;
        obj["type"] = "log";
        obj["message"] = msg;
        emit broadcastToAdmins(obj);
    });

    connect(this, &ServerManager::clientCountChanged, this, [this](int count) {
        QJsonObject obj;
        obj["type"] = "client_count";
        obj["count"] = count;
        emit broadcastToAdmins(obj);
    });

    connect(this, &ServerManager::databaseUpdated, this, [this](const QString &type) {
        QJsonObject obj;
        obj["type"] = "table_refresh_required";
        obj["target_table"] = type;
        emit broadcastToAdmins(obj);
    });
}

bool ServerManager::startServer(int port)
{
    QSqlQuery walQuery;
    if (!walQuery.exec("PRAGMA journal_mode=WAL;")) {
        qWarning() << "Error enabling database WAL mode:" << walQuery.lastError().text();
    } else {
        qDebug() << "Database optimized: WAL mode enabled for thread concurrency management.";
    }

    if (!this->listen(QHostAddress::Any, port)) {
        qWarning() << "Server failed to start on port" << port << ": " << this->errorString();
        return false;
    }

    qDebug() << "BookClub server successfully started on port" << port << "...";
    return true;
}

void ServerManager::stopServer()
{
    if (this->isListening()) {
        this->close();
        qDebug() << "Server stopped listening on port" << this->serverPort();
    }
}

void ServerManager::incomingConnection(qintptr socketDescriptor)
{
    ClientHandler *handler = new ClientHandler(socketDescriptor,this);
    QString connectLog = QString("<font color='#7f8c8d'><b>[SYS]</b></font> New client connected. User: <b>Anonymous</b> | Descriptor: %1")
                             .arg(socketDescriptor);
    emit serverLogEvent(connectLog);

    m_activeClients++;
    emit clientCountChanged(m_activeClients);

    connect(handler,&ClientHandler::logProduced,this,&ServerManager::serverLogEvent);
    connect(handler, &ClientHandler::databaseUpdated,this,&ServerManager::databaseUpdated);
    connect(handler, &ClientHandler::broadcastTargetedUpdate, this, &ServerManager::onBroadcastReceived);
    connect(this, &ServerManager::sendToAllClientsSignal, handler, &ClientHandler::sendToClient);

    connect(handler, &ClientHandler::clientDisconnectedSignal,this,[this](qintptr desc,const QString &username){
        if(m_activeClients > 0) m_activeClients--;
        emit clientCountChanged(m_activeClients);
        emit serverLogEvent(QString("<font color='#7f8c8d'><b>[SYS]</b></font> User <b>%1</b> disconnected.").arg(username));
    });

    connect(handler, &ClientHandler::finished, handler, &ClientHandler::deleteLater);
    handler->start();
}

void ServerManager::onBroadcastReceived(const QJsonObject &msg)
{
    emit sendToAllClientsSignal(msg);
}
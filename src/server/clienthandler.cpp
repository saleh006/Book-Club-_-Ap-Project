#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "clienthandler.h"

ClientHandler::ClientHandler(qintptr socketDescriptor, QObject *parent)
    : QThread(parent), m_socketDescriptor(socketDescriptor), m_socket(nullptr)
{}

ClientHandler::~ClientHandler()
{
    if (this->isRunning()) {
        this->quit();
        this->wait();
    }
}

void ClientHandler::run()
{
    qDebug() << "New thread created for client. Thread ID:" << QThread::currentThreadId();

    m_socket = new QTcpSocket();

    if (!m_socket->setSocketDescriptor(m_socketDescriptor)) {
        qWarning() << "Error setting client socket descriptor!";
        delete m_socket;
        m_socket = nullptr;
        return;
    }
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead, Qt::DirectConnection);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected, Qt::DirectConnection);

    exec();
}

void ClientHandler::onReadyRead()
{
    m_buffer.append(m_socket->readAll());
    int newlineIndex;
    while ((newlineIndex = m_buffer.indexOf('\n')) != -1) {
        QByteArray rawData = m_buffer.left(newlineIndex).trimmed();
        m_buffer.remove(0, newlineIndex + 1);
        if (rawData.isEmpty()) continue;
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(rawData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            m_socket->write("{\"status\":\"error\",\"message\":\"Invalid JSON format\"}\n");
            m_socket->flush();
            continue;
        }
        QJsonObject requestObj = jsonDoc.object();
        QString action = requestObj["action"].toString();
        QJsonObject responseObj;
        QString clientIp = m_socket ? m_socket->peerAddress().toString() : "Unknown IP";
        QString reqLog = QString("<font color='#3498db'><b>[REQ]</b></font> "
                                 "User: <b>%1</b> | IP: %2 | Action: <font color='#f1c40f'><b>%3</b></font>")
                             .arg(m_username)
                             .arg(clientIp)
                             .arg(action);
        emit logProduced(reqLog);

        bool handled = handleUserActions(action, requestObj, responseObj)
                       || handleBookActions(action, requestObj, responseObj)
                       || handleDiscountAndWishlistActions(action, requestObj, responseObj)
                       || handleReviewActions(action, requestObj, responseObj)
                       || handleNotificationActions(action, requestObj, responseObj)
                       || handleShelfAndProgressActions(action, requestObj, responseObj)
                       || handleCartActions(action, requestObj, responseObj)
                       || handlePurchaseActions(action, requestObj, responseObj)
                       || handlePublisherActions(action, requestObj, responseObj);

        if (!handled) {
            responseObj["status"] = "error";
            responseObj["message"] = "Invalid action.";
        }

        QJsonDocument responseDoc(responseObj);
        m_socket->write(responseDoc.toJson(QJsonDocument::Compact) + "\n");
        m_socket->flush();

        QString status = responseObj["status"].toString();
        QString msg = responseObj["message"].toString();
        QString resLog;
        if (status == "success") {
            resLog = QString("<font color='#2ecc71'><b>[RES]</b></font> Action: <b>%1</b> | Status: SUCCESS")
            .arg(action);
            if (!msg.isEmpty()) resLog += QString(" (%1)").arg(msg);
        }
        else {
            resLog = QString("<font color='#e74c3c'><b>[ERR]</b></font> Action: <b>%1</b> | Status: ERROR | Reason: <i>%2</i>")
            .arg(action)
                .arg(msg.isEmpty() ? "Unknown Error" : msg);
        }
        QString finalLog = resLog + "<hr style='border: 0; border-top: 1px solid #3a3a4c; margin: 4px 0;'>";
        emit logProduced(finalLog);
    }
}

void ClientHandler::onDisconnected(){
    qDebug() << "Client disconnected. Cleaning up memory...";
    emit clientDisconnectedSignal(m_socketDescriptor,m_username);
    if(m_socket){
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    quit();
}

void ClientHandler::sendToClient(const QJsonObject &msg)
{
    if (m_socket && m_socket->isOpen()) {
        m_socket->write(QJsonDocument(msg).toJson(QJsonDocument::Compact) + "\n");
        m_socket->flush();
    }
}

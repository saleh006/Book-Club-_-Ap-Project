#ifndef SERVERMANAGER_H
#define SERVERMANAGER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QList>

class ServerManager : public QTcpServer
{
    Q_OBJECT
public:
    explicit ServerManager(QObject *parent = nullptr);
    bool startServer(int port);
    void stopServer();

signals:
    void serverLogEvent(const QString &message);
    void clientCountChanged(int count);
    void databaseUpdated(const QString &type);
    void broadcastToAdmins(const QJsonObject &msg);
    void sendToAllClientsSignal(const QJsonObject &msg);
public slots:
    void onBroadcastReceived(const QJsonObject &msg);
protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    int m_activeClients = 0;
};

#endif // SERVERMANAGER_H
#ifndef SERVERMANAGER_H
#define SERVERMANAGER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

class ClientHandler : public QThread
{
    Q_OBJECT
public:
    explicit ClientHandler(qintptr socketDescriptor, QObject *parent = nullptr);
    ~ClientHandler();

signals:
    void logProduced(const QString &message);
    void clientDisconnectedSignal(qintptr descriptor, const QString &username);
    void databaseUpdated(const QString &type);

protected:
    void run() override;

private slots:
    void onReadyRead();
    void onDisconnected();

public slots:
    void sendToClient(const QJsonObject &msg);

private:
    qintptr m_socketDescriptor;
    QTcpSocket *m_socket;
    QByteArray m_buffer;
    QString m_username = "Anonymous";
};

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

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    int m_activeClients = 0;
};

#endif // SERVERMANAGER_H
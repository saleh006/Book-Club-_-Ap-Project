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

protected:
    void run() override;

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    qintptr m_socketDescriptor;
    QTcpSocket *m_socket;
};

class ServerManager : public QTcpServer
{
    Q_OBJECT
public:
    explicit ServerManager(QObject *parent = nullptr);
    bool startServer(int port);

protected:
    void incomingConnection(qintptr socketDescriptor) override;
};

#endif // SERVERMANAGER_H
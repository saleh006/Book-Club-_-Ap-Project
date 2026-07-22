#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QThread>
#include <QTcpSocket>
#include <QJsonObject>
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
    bool handleUserActions(const QString &action, const QJsonObject &requestObj, QJsonObject &responseObj);
    bool handleBookActions(const QString &action, const QJsonObject &requestObj, QJsonObject &responseObj);
    bool handleDiscount_Wishlist_ReviewsActions(const QString &action, const QJsonObject &requestObj, QJsonObject &responseObj);
    bool handleReviewAndNotificationActions(const QString &action, const QJsonObject &requestObj, QJsonObject &responseObj);
    bool handleNotificationActions(const QString &action, const QJsonObject &requestObj, QJsonObject &responseObj);
    bool handleCart_PurchaseActions(const QString &action, const QJsonObject &requestObj, QJsonObject &responseObj);
    bool handlePublisherActions(const QString &action, const QJsonObject &requestObj, QJsonObject &responseObj);

    qintptr m_socketDescriptor;
    QTcpSocket *m_socket;
    QByteArray m_buffer;
    QString m_username = "Anonymous";
};

#endif // CLIENTHANDLER_H

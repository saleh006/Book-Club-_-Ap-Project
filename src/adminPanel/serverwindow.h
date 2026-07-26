#ifndef SERVERWINDOW_H
#define SERVERWINDOW_H

#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QTcpSocket>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

class ServerWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ServerWindow(QWidget *parent = nullptr);
    ~ServerWindow();

private slots:
    void onNewLogReceived(const QString &message);
    void onClientCountUpdated(int count);
    void updateSystemUsage();
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    QTimer *m_sysTimer;
    QTcpSocket *m_socket;
    QTextEdit *m_logDisplay;
    QLabel *m_statusLabel;
    QLabel *m_clientCountLabel;
    QLabel *m_cpuLabel;
    QLabel *m_ramLabel;
    void setupUi();
    QTimer *m_reconnectTimer;
    QWidget* createStatCard(const QString &iconPath, QLabel *&textLabel,
                            const QString &initialText, const QString &textColor);

#ifdef Q_OS_WIN
    FILETIME m_preIdleTime;
    FILETIME m_preKernelTime;
    FILETIME m_preUserTime;
    double getCpuUsage();
#endif
};

#endif // SERVERWINDOW_H
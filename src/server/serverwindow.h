#ifndef SERVERWINDOW_H
#define SERVERWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "servermanager.h" // سرور منیجر تو را این‌جا اینکلود می‌کنیم

class ServerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ServerWindow(ServerManager *server, QWidget *parent = nullptr);
    ~ServerWindow();

private slots:
    void onNewLogReceived(const QString &message);
    void onClientCountUpdated(int count);

private:
    ServerManager *m_serverManager;
    QTextEdit *m_logDisplay;
    QLabel *m_statusLabel;
    QLabel *m_clientCountLabel;
    QLabel *m_cpuLabel;
    QLabel *m_ramLabel;

    void setupUi();
};

#endif // SERVERWINDOW_H
#ifndef SERVERWINDOW_H
#define SERVERWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QTableWidget>
#include "servermanager.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

class ServerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ServerWindow(ServerManager *server, QWidget *parent = nullptr);
    ~ServerWindow();

private slots:
    void onNewLogReceived(const QString &message);
    void onClientCountUpdated(int count);
    void updateSystemUsage();
    void loadUsersFromDatabase();
    void loadBooksFromDatabase();

private:
    QTimer *m_sysTimer;
    ServerManager *m_serverManager;
    QTextEdit *m_logDisplay;
    QLabel *m_statusLabel;
    QLabel *m_clientCountLabel;
    QLabel *m_cpuLabel;
    QLabel *m_ramLabel;
    QTableWidget *usersTable;
    QTableWidget *booksTable;
    void setupUi();

#ifdef Q_OS_WIN
    FILETIME m_preIdleTime;
    FILETIME m_preKernelTime;
    FILETIME m_preUserTime;
    double getCpuUsage();
#endif
};

#endif // SERVERWINDOW_H
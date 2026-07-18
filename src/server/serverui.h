#ifndef SERVERUI_H
#define SERVERUI_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include "servermanager.h"
#include "databasemanager.h"

class ServerUi : public QWidget
{
    Q_OBJECT

public:
    explicit ServerUi(QWidget *parent = nullptr);
    ~ServerUi();

private slots:
    void toggleServer();

private:
    QLabel *m_statusLabel;
    QPushButton *m_toggleButton;
    ServerManager *m_server;
    bool m_isRunning;

    void setupUI();
};

#endif // SERVERUI_H
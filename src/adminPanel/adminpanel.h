#ifndef ADMINPANEL_H
#define ADMINPANEL_H

#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QLineEdit>
#include "serverwindow.h"

class AdminPanel : public QWidget
{
    Q_OBJECT
public:
    explicit AdminPanel(QWidget *parent = nullptr);

signals:
    void logoutRequested();

private slots:
    void switchPage(int index);

    void filterUsers(const QString &text);
    void handleBlockUser();
    void handleUnblockUser();

private:
    QStackedWidget *m_stackedWidget;
    QPushButton *m_btnMonitor;
    QPushButton *m_btnUsers;
    QPushButton *m_btnBooks;
    QPushButton *m_btnLogout;

    QTableWidget *m_usersTable;
    QLineEdit *m_searchEdit;
    QPushButton *m_btnBlock;
    QPushButton *m_btnUnblock;
    QWidget* createUsersPage();

    void setupUi();
    void updateButtonStyles(int currentIndex);
};

#endif // ADMINPANEL_H
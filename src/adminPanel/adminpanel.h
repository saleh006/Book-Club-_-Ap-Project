#ifndef ADMINPANEL_H
#define ADMINPANEL_H

#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QLineEdit>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
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

    void filterBooks(const QString &text);
    void handleApproveBook();
    void handleRejectBook();

    void onReadyRead();

private:
    QTcpSocket *m_socket;
    void updateRowAppearance(QTableWidget *table, int row, bool isDimmed);

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

    QTableWidget *m_booksTable;
    QLineEdit *m_bookSearchEdit;
    QPushButton *m_btnApprove;
    QPushButton *m_btnReject;
    QWidget* createBooksPage();

    void setupUi();
    void updateButtonStyles(int currentIndex);
protected:
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // ADMINPANEL_H
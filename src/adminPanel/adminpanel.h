#ifndef ADMINPANEL_H
#define ADMINPANEL_H

#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include "serverwindow.h"
#include "usertab.h"
#include "publisherstab.h"
#include "booktab.h"
#include "reviewstab.h"

class AdminPanel : public QWidget
{
    Q_OBJECT
public:
    explicit AdminPanel(QWidget *parent = nullptr);

signals:
    void logoutRequested();

private slots:
    void switchPage(int index);
    void onReadyRead();

private:
    QTcpSocket *m_socket;
    void updateRowAppearance(QTableWidget *table, int row, bool isDimmed);

    QStackedWidget *m_stackedWidget;
    QPushButton *m_btnMonitor;
    QPushButton *m_btnUsers;
    QPushButton *m_btnBooks;
    QPushButton *m_btnLogout;
    QPushButton *m_btnPublishers;
    QPushButton *m_btnReviews;

    UsersTab *m_userTab;
    BookTab *m_booksTab;
    PublishersTab *m_publishersTab;
    ReviewsTab *m_reviewsTab;

    void setupUi();
    void updateButtonStyles(int currentIndex);
protected:
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // ADMINPANEL_H
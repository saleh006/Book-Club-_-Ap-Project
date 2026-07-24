#ifndef PUBLISHERPANEL_H
#define PUBLISHERPANEL_H

#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QScrollArea>
#include <QTcpSocket>
#include <QtCharts/QChartView>
#include <QTableWidget>
#include <QComboBox>
#include <QMap>
#include <QListWidget>
#include "models.h"
#include "QLineEdit"
#include "src/userPanel/notificationtoast.h"

class PublisherPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PublisherPanel(int publisherId, const QString &fullName, const QString &username, QWidget *parent = nullptr);

signals:
    void logoutRequested();

private slots:
    void switchPage(int index);
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void handleAddBook();
    void handleEditBook(int bookId);
    void handleDeleteBook(int bookId);
    void handleToggleActive(int bookId, int newStatus);
    void handleSetOffer(int bookId);
    void filterBooks(const QString &text);
    void handleEditProfile();

private:
    void setupUi();
    QWidget *createStatsPage();
    QWidget *createBooksPage();
    void updateButtonStyles(int currentIndex);
    void requestBooks();
    void requestStats();
    void populateBooksGrid(const QVector<Book> &books);
    void sendRequest(const QJsonObject &requestObj);

    int m_publisherId;
    QVector<Book> m_currentBooks;

    QTcpSocket *m_socket;

    QStackedWidget *m_stackedWidget;
    QPushButton *m_btnStats;
    QPushButton *m_btnBooks;
    QPushButton *m_btnLogout;

    //Notification
    QPushButton *m_btnNotifications;
    QLabel *m_notifBadge;
    QVector<Notification> m_notifications;
    QListWidget *m_notifListWidget;
    QWidget *m_notifPage;

    QWidget *createNotificationsPage();
    void requestNotifications();
    void rebuildNotificationList();
    void updateNotificationBadge();
    void showNotificationToast(const QString &title, const QString &message);

    ////////////////////////

    QLabel *m_statBookCount;
    QLabel *m_statTotalSales;
    QLabel *m_statAvgRating;
    QLabel *m_statTotalIncome;
    QVector<Book> m_allBooks; // full list from server, unfiltered
    QLineEdit *m_bookSearchEdit;
    QScrollArea *m_booksScrollArea;
    QWidget *m_booksGridContainer;
    QGridLayout *m_booksGrid;
    QLabel *m_nameLabel;
    QLabel *m_usernameLabel;
    QString m_fullName;
    QString m_username;
    QString m_email;

    QWidget *makeStatCard(const QString &icon, const QString &iconBg,
                          const QString &title, const QString &subtitle, QLabel *&valueOut);
    QWidget *makeSectionCard(const QString &icon, const QString &title,
                             QWidget *content, QWidget *headerRight = nullptr);
    QTableWidget *makeTopTable();
    void fillTopTable(QTableWidget *table, const QVector<Book> &books);
    void updateDashboard();                       // tables + bar/pie charts from m_allBooks
    void updateSalesTrend(const QJsonArray &pts); // line chart
    void requestSalesTrend();
    static void replaceChart(QChartView *view, QChart *chart);

    QLabel *m_welcomeLabel = nullptr;
    QTableWidget *m_bestTable = nullptr;
    QTableWidget *m_worstTable = nullptr;
    QChartView *m_trendView = nullptr;
    QChartView *m_cmpView = nullptr;
    QChartView *m_ratingView = nullptr;
    QChartView *m_pieView = nullptr;
    QVBoxLayout *m_pieLegendLayout = nullptr;
    QComboBox *m_trendCombo = nullptr;
    QMap<int, double> m_incomeByBookId;
};

#endif
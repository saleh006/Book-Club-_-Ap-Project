#ifndef PUBLISHERPANEL_H
#define PUBLISHERPANEL_H

#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QScrollArea>
#include <QTcpSocket>
#include "models.h"
#include "QLineEdit"

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
};

#endif
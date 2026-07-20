#ifndef USERPANEL_H
#define USERPANEL_H

#include <QWidget>
#include <QTcpSocket>
#include <QScrollArea>
#include <QLabel>
#include <QLineEdit>
#include <QStackedWidget>
#include <QVector>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <QTimer>
#include <QGridLayout>
#include "models.h"

class UserPanel : public QWidget
{
    Q_OBJECT
public:
    explicit UserPanel(int userId, const QString &fullName, const QString &username, QWidget *parent = nullptr);
    ~UserPanel() = default;

signals:
    void logoutRequested();

private slots:
    void onReadyRead();
    void onSocketError();
    void handleEditProfile();

private:
    void setupUi();
    void sendRequest(const QJsonObject &requestObj);
    void switchPage(int index);
    void updateHero();
    void rebuildHomeSections();
    QWidget *makeHorizontalScrollRow(const QString &title, QHBoxLayout *&rowLayoutOut);


    // Home Page Layout & Mechanics
    QWidget *createHomePage();
    QWidget *makeBookCard(const Book &b);
    QWidget *makeBookRow(const QString &title, QHBoxLayout *&rowLayoutOut);
    void fillBookRow(QHBoxLayout *rowLayout, const QVector<Book> &books, int maxCount = 6);
    void requestAllBooks();

    // Functional Actions / Stubs
    void openBookDetails(int bookId);
    void addToCart(int bookId);
    void openGenre(const QString &g);

    // User Session Fields
    int m_userId;
    QString m_fullName;
    QString m_username;
    QString m_email;
    QTcpSocket *m_socket;

    // Sidebar & Base Widgets
    QLabel *m_nameLabel = nullptr;
    QLabel *m_usernameLabel = nullptr;
    QPushButton *m_btnHome = nullptr;
    QPushButton *m_btnLogout = nullptr;
    QStackedWidget *m_stackedWidget = nullptr;

    // Store State Containers
    QVector<Book> m_storeBooks;
    QStringList m_favoriteGenres;

    // Hero Banner Controls
    QVector<Book> m_heroBooks;
    int m_heroIndex = 0;
    QLabel *m_heroCover = nullptr;
    QLabel *m_heroGenre = nullptr;
    QLabel *m_heroTitle = nullptr;
    QLabel *m_heroAuthor = nullptr;
    QLabel *m_heroRating = nullptr;
    QLabel *m_heroDesc = nullptr;

    // Catalog Row Layout References
    QHBoxLayout *m_rowRecommended = nullptr;
    QHBoxLayout *m_rowNewReleases = nullptr;
    QHBoxLayout *m_rowBestsellers = nullptr;
    QHBoxLayout *m_rowFree = nullptr;
    QLineEdit *m_searchEdit = nullptr;

    void setupSearch();
    void runSearch(const QString &text);
    QWidget *m_homeSections = nullptr;     // wraps hero + categories + rows
    QWidget *m_searchResultsPanel = nullptr;
    QGridLayout *m_searchResultsGrid = nullptr;
    QLabel *m_searchResultsLabel = nullptr;
    QTimer *m_searchTimer = nullptr;
};

#endif // USERPANEL_H
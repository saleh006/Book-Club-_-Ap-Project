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
#include <functional>
#include "models.h"
#include "shoppingcarttab.h"
#include "bookdetailspage.h"
#include "wishlistpage.h"

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
    QWidget *makeHorizontalScrollRow(const QString &title, QHBoxLayout *&rowLayoutOut,
                                     std::function<QVector<Book>()> getFullList);


    // Home Page Layout & Mechanics
    QWidget *createHomePage();
    QWidget *makeBookCard(const Book &b);
    QWidget *makeBookRow(const QString &title, QHBoxLayout *&rowLayoutOut);
    void fillBookRow(QHBoxLayout *rowLayout, const QVector<Book> &books, int maxCount = 6);
    void requestAllBooks();

    // Functional Actions / Stubs
    void openBookDetails(int bookId);
    void addToCart(int bookId);
    void toggleWishlist(int bookId);
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
    QPushButton *m_btnCart = nullptr;
    QPushButton *m_heroCartBtn = nullptr;
    QPushButton *m_heroWishlistBtn = nullptr;
    QPushButton *m_btnWishlist = nullptr;
    ShoppingCartPage *m_cartPage = nullptr;
    WishlistPage *m_wishlistPage = nullptr;
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
    QPushButton *m_searchResultsCloseBtn = nullptr;
    void showResultsGrid(const QString &headerText, const QVector<Book> &results);
    void closeResultsView();
    QWidget *m_homeSections = nullptr;     // wraps hero + categories + rows
    QWidget *m_searchResultsPanel = nullptr;
    QGridLayout *m_searchResultsGrid = nullptr;
    QLabel *m_searchResultsLabel = nullptr;
    QTimer *m_searchTimer = nullptr;

    BookDetailsPage *m_detailsPage = nullptr;

    QVector<Book> m_recommendedBooks;
    QVector<Book> m_newestBooks;
    QVector<Book> m_bestsellerBooks;
    QVector<Book> m_freeBooks;

    void showFullList(const QString &title, const QVector<Book> &books);
};

#endif // USERPANEL_H
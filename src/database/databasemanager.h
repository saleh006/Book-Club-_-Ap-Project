#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H
#include <QSqlDatabase>
#include <QString>
#include <QDate>
#include <QThread>
#include <QMutex>
#include <models.h>

struct User
{
    int id = -1;
    QString username;
    QString passwordHash;
    QString fullName;
    QString email;
    bool isBlocked = false;
    QDate registerDate;
    QString role;
    QString recoveryAnswer;
};

// struct Book {
//     int id = -1;
//     int publisherId = -1; // we should add this in uml
//     QString title;
//     QString author;
//     QString genre;
//     QString description;
//     double price = 0.0;
//     QString coverImagePath;
//     QString pdfPath;
//     bool isActive = true;
//     double averageRating = 0.0;
//     int totalSales = 0;
// };

// struct Discount {
//     int id = -1;
//     int bookId = -1;
//     QString type;
//     double value = 0.0;
//     QDateTime startDate;
//     QDateTime endDate;
// };

// struct Review {
//     int id = -1;
//     QString comment;
//     int rating = 0;
//     QDateTime date;
//     int userId = -1;
//     int bookId = -1;
// };

// struct Notification {
//     int id = -1;
//     int userId = -1;
//     QString title;
//     QString message;
//     QDateTime date;
//     bool isRead = false;
// };

// struct Shelf {
//     int id = -1;
//     int libraryUserId = -1;
//     QString title;
// };

// struct ReadingProgress {
//     int bookId = -1;
//     int userId = -1;
//     int lastPage = 0;
// };

// struct Purchase {
//     int id = -1;
//     int userId = -1;
//     double totalPrice = 0.0;
//     QDateTime purchaseDate;
//     QVector<int> bookIds;
// };

// struct CartItem {
//     int bookId = -1;
//     int quantity = 1;
//     double price = 0.0;
// };

struct UserProfileSummary {
    User user;
    QVector<Book> ownedBooks;
    QVector<Book> wishlist;
    QVector<CartItem> cartItems;
    double cartTotal = 0.0;
    QVector<Purchase> purchaseHistory;
};

struct PublisherProfileSummary {
    User user;
    QVector<Book> publishedBooks;
    double totalIncome = 0.0;
    int bookCount = 0;
    int totalSales = 0;
    double averageRating = 0.0;
};

class DatabaseManager
{
public:
    static DatabaseManager &instance();
    bool initialize(const QString &dbPath);

    //user

    bool registerUser(const QString &username,
                      const QString &password,
                      const QString &fullName,
                      const QString &email,
                      const QString &recoveryAnswer,
                      QString &errorMsg,
                      const QString &role = "user");
    bool authenticateUser(const QString &username,
                          const QString &password,
                          QString &errorMsg,
                          User *outUser = nullptr);
    bool fetchUser(const QString &username, User &outUser, QString &errorMsg);
    bool setUserBlocked(const QString &username, bool blocked, QString &errorMsg);
    bool resetPasswordWithRecovery(const QString &username,
                                   const QString &recoveryAnswer,
                                   const QString &newPassword,
                                   QString &errorMsg);
    bool fetchUserProfileForAdmin(const QString &username, UserProfileSummary &outProfile, QString &errorMsg);
    bool fetchAllUsersProfilesForAdmin(QVector<UserProfileSummary>& outProfiles , QString &errorMsg);
    bool updateUserProfile(const QString &username, const QString &fullName,
                           const QString &email, QString &errorMsg);
    bool updateUserProfile(int userId, const QString &newUsername,
                           const QString &fullName, const QString &email, QString &errorMsg);
    bool changePassword(int userId, const QString &oldPassword,
                        const QString &newPassword, QString &errorMsg);

    //book

    bool addBook(const Book &book, int &newBookId, QString &errorMsg);
    bool updateBook(const Book &book, QString &errorMsg);
    bool deleteBook(int bookId, QString &errorMsg);
    bool fetchBook(int bookId, Book &outBook, QString &errorMsg);
    bool fetchAllBooks(QVector<Book> &outBooks, QString &errorMsg, bool activeOnly = true);
    bool fetchBooksByGenre(const QString &genre, QVector<Book> &outBooks, QString &errorMsg);
    bool setBookStatus(int bookId, int status, QString &errorMsg); // 1=active, 0=inactive/pending, -1=deleted

    //discounts

    bool addDiscount(const Discount &discount, QString &errorMsg);
    bool fetchActiveDiscount(int bookId, Discount &outDiscount, QString &errorMsg);

    //reviews
    bool addReview(const Review &review, QString &errorMsg);
    bool fetchReviewsForBook(int bookId, QVector<Review> &outReviews, QString &errorMsg);
    bool recalculateAverageRating(int bookId, QString &errorMsg);

    // Notifications
    bool addNotification(int userId, const QString &title, const QString &message, QString &errorMsg);
    bool fetchNotifications(int userId, QVector<Notification> &outNotifications, QString &errorMsg);
    bool markNotificationRead(int notificationId, QString &errorMsg);

    // Library / shelves / reading progress
    bool createShelf(int userId, const QString &title, int &newShelfId, QString &errorMsg);
    bool addBookToShelf(int shelfId, int bookId, QString &errorMsg);
    bool fetchShelves(int userId, QVector<Shelf> &outShelves, QString &errorMsg);
    bool fetchShelfBooks(int shelfId, QVector<Book> &outBooks, QString &errorMsg);
    bool updateReadingProgress(int userId, int bookId, int lastPage, QString &errorMsg);
    bool fetchReadingProgress(int userId, int bookId, ReadingProgress &outProgress, QString &errorMsg);
    bool fetchOwnedBooks(int userId, QVector<Book> &outBooks, QString &errorMsg);

    // wishlist
    bool addToWishlist(int userId, int bookId, QString &errorMsg);
    bool removeFromWishlist(int userId, int bookId, QString &errorMsg);
    bool fetchWishlist(int userId, QVector<Book> &outBooks, QString &erroirMsg);

    // Shopping cart
    bool addToCart(int userId, int bookId, int quantity, QString &errorMsg);
    bool removeFromCart(int userId, int bookId, QString &errorMsg);
    bool clearCart(int userId, QString &errorMsg);
    bool fetchCart(int userId, QVector<CartItem> &outItems, double &totalPrice, QString &errorMsg);

    // Purchases
    bool checkoutCart(int userId, QString &errorMsg, int &purchaseId);
    bool fetchPurchaseHistory(int userId, QVector<Purchase> &outPurchases, QString &errorMsg);

    //publisher
    bool fetchPublishedBooks(int publisherId, QVector<Book> &outBooks, QString &errorMsg, bool activeOnly = false);
    bool fetchPublisherIncome(int publisherId, double &outTotalIncome, QString &errorMsg);
    bool fetchPublisherIncomeForBook(int publisherId, int bookId, double &outIncome, QString &errorMsg);
    bool fetchPublisherStats(int publisherId, int &outBookCount, int &outTotalSales, double &outAverageRating, QString &errorMsg);
    bool setBookOwnership(int bookId, int publisherId, QString &errorMsg);    // reassign/transfer
    bool fetchPublisherProfileForAdmin(const QString &username, PublisherProfileSummary &outProfile, QString &errorMsg);
    bool fetchAllPublisherProfilesForAdmin(QVector<PublisherProfileSummary> &outProfiles, QString &errorMsg);
    bool fetchPublisherSalesTrend(int publisherId, const QString &granularity,
                                  QVector<QPair<QString, int>> &outPoints, QString &errorMsg);


private:
    DatabaseManager() = default;
    bool createTableForUser();
    bool createTableForBooks();
    bool createTableForDiscounts();
    bool createTableForReviews();
    bool createTableForNotifications();
    bool createTableForShelves();
    bool createTableForReadingProgress();
    bool createTableForWishlist();
    bool createTableForCart();
    bool createTableForPurchases();
    bool createAllTables();
    QString generateSalt() const;
    QString hashPassword(const QString &password, const QString &salt) const;
    QSqlDatabase database() const;
    QSqlDatabase m_db;
    QString m_dbPath;
    bool seedAdminAccount();
};

#endif // DATABASEMANAGER_H

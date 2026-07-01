#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H
#include <QSqlDatabase>
#include <QString>
#include <QDate>

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

struct Book {
    int id = -1;
    int publisherId = -1; // we should add this in uml
    QString title;
    QString author;
    QString genre;
    QString description;
    double price = 0.0;
    QString coverImagePath;
    QString pdfPath;
    bool isActive = true;
    double averageRating = 0.0;
    int totalSales = 0;
};

struct Discount {
    int id = -1;
    int bookId = -1;
    QString type;
    double value = 0.0;
    QDateTime startDate;
    QDateTime endDate;
};

struct Review {
    int id = -1;
    QString comment;
    int rating = 0;
    QDateTime date;
    int userId = -1;
    int bookId = -1;
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

    //book

    bool addBook(const Book &book, int &newBookId, QString &errorMsg);
    bool updateBook(const Book &book, QString &errorMsg);
    bool deleteBook(int bookId, QString &errorMsg);
    bool fetchBook(int bookId, Book &outBook, QString &errorMsg);
    bool fetchAllBooks(QVector<Book> &outBooks, QString &errorMsg, bool activeOnly = true);
    bool fetchBooksByGenre(const QString &genre, QVector<Book> &outBooks, QString &errorMsg);

    //discounts

    bool addDiscount(const Discount &discount, QString &errorMsg);
    bool fetchActiveDiscount(int bookId, Discount &outDiscount, QString &errorMsg);

    //reviews
    bool addReview(const Review &review, QString &errorMsg);
    bool fetchReviewsForBook(int bookId, QVector<Review> &outReviews, QString &errorMsg);
    bool recalculateAverageRating(int bookId, QString &errorMsg);

private:
    DatabaseManager() = default;
    bool createTableForUser();
    bool createTableForBooks();
    bool createTableForDiscounts();
    bool createTableForReviews();
    QString generateSalt() const;
    QString hashPassword(const QString &password, const QString &salt) const;

    QSqlDatabase m_db;
};

#endif // DATABASEMANAGER_H

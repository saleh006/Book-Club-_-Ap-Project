#ifndef MODELS_H
#define MODELS_H

#include <QString>
#include <QDateTime>
#include <QVector>

struct Book {
    int id = -1;
    int publisherId = -1;
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

struct Notification {
    int id = -1;
    int userId = -1;
    QString title;
    QString message;
    QDateTime date;
    bool isRead = false;
};

struct Shelf {
    int id = -1;
    int libraryUserId = -1;
    QString title;
};

struct ReadingProgress {
    int bookId = -1;
    int userId = -1;
    int lastPage = 0;
};

struct Purchase {
    int id = -1;
    int userId = -1;
    double totalPrice = 0.0;
    QDateTime purchaseDate;
    QVector<int> bookIds;
};

struct CartItem {
    int bookId = -1;
    int quantity = 1;
    double price = 0.0;
};

#endif // MODELS_H
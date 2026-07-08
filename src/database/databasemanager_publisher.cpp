#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>

bool DatabaseManager::fetchPublishedBooks(int publisherId, QVector<Book> &outBooks, QString &errorMsg, bool activeOnly)
{
    QSqlQuery query(database());
    query.prepare(activeOnly
                      ? "SELECT * FROM books WHERE publisher_id = :pid AND is_active = 1"
                      : "SELECT * FROM books WHERE publisher_id = :pid");
    query.bindValue(":pid", publisherId);
    if (!query.exec()) {
        errorMsg = "Database error while fetching published books: " + query.lastError().text();
        return false;
    }
    outBooks.clear();
    while (query.next()) {
        Book b;
        b.id = query.value("id").toInt();
        b.publisherId = publisherId;
        b.title = query.value("title").toString();
        b.author = query.value("author").toString();
        b.genre = query.value("genre").toString();
        b.description = query.value("description").toString();
        b.price = query.value("price").toDouble();
        b.coverImagePath = query.value("cover_image_path").toString();
        b.pdfPath = query.value("pdf_path").toString();
        b.isActive = query.value("is_active").toBool();
        b.averageRating = query.value("average_rating").toDouble();
        b.totalSales = query.value("total_sales").toInt();
        outBooks.push_back(b);
    }
    return true;
}

bool DatabaseManager::fetchPublisherIncome(int publisherId, double &outTotalIncome, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT COALESCE(SUM(pi.price_paid), 0) AS income
        FROM purchase_items pi
        JOIN books b ON b.id = pi.book_id
        WHERE b.publisher_id = :pid
    )");
    query.bindValue(":pid", publisherId);
    if (!query.exec()) {
        errorMsg = "Database error while calculating publisher income: " + query.lastError().text();
        return false;
    }
    outTotalIncome = query.next() ? query.value("income").toDouble() : 0.0;
    return true;
}

bool DatabaseManager::fetchPublisherIncomeForBook(int publisherId, int bookId, double &outIncome, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT COALESCE(SUM(pi.price_paid), 0) AS income
        FROM purchase_items pi
        JOIN books b ON b.id = pi.book_id
        WHERE b.publisher_id = :pid AND b.id = :bid
    )");
    query.bindValue(":pid", publisherId);
    query.bindValue(":bid", bookId);
    if (!query.exec()) {
        errorMsg = "Database error while calculating book income: " + query.lastError().text();
        return false;
    }
    outIncome = query.next() ? query.value("income").toDouble() : 0.0;
    return true;
}

bool DatabaseManager::fetchPublisherStats(int publisherId, int &outBookCount, int &outTotalSales,
                                          double &outAverageRating, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT COUNT(*) AS bookCount,
               COALESCE(SUM(total_sales), 0) AS totalSales,
               COALESCE(AVG(average_rating), 0) AS avgRating
        FROM books
        WHERE publisher_id = :pid AND is_active = 1
    )");
    query.bindValue(":pid", publisherId);
    if (!query.exec()) {
        errorMsg = "Database error while fetching publisher stats: " + query.lastError().text();
        return false;
    }
    if (!query.next()) {
        outBookCount = 0;
        outTotalSales = 0;
        outAverageRating = 0.0;
        return true;
    }
    outBookCount = query.value("bookCount").toInt();
    outTotalSales = query.value("totalSales").toInt();
    outAverageRating = query.value("avgRating").toDouble();
    return true;
}

bool DatabaseManager::setBookOwnership(int bookId, int publisherId, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("UPDATE books SET publisher_id = :pid WHERE id = :bid");
    query.bindValue(":pid", publisherId);
    query.bindValue(":bid", bookId);
    if (!query.exec()) {
        errorMsg = "Failed to reassign book: " + query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        errorMsg = "Book not found.";
        return false;
    }
    return true;
}

bool DatabaseManager::fetchPublisherProfileForAdmin(const QString &username, PublisherProfileSummary &outProfile, QString &errorMsg)
{
    if (!fetchUser(username, outProfile.user, errorMsg)) {
        return false;
    }

    if (outProfile.user.role != "publisher") {
        errorMsg = "This user is not a publisher.";
        return false;
    }

    const int publisherId = outProfile.user.id;

    QString stepError;
    if (!fetchPublishedBooks(publisherId, outProfile.publishedBooks, stepError, false))
        qWarning() << "Failed to fetch published books for admin view:" << stepError;
    if (!fetchPublisherIncome(publisherId, outProfile.totalIncome, stepError))
        qWarning() << "Failed to fetch publisher income for admin view:" << stepError;
    if (!fetchPublisherStats(publisherId, outProfile.bookCount, outProfile.totalSales, outProfile.averageRating, stepError))
        qWarning() << "Failed to fetch publisher stats for admin view:" << stepError;
    return true;
}

bool DatabaseManager::fetchAllPublisherProfilesForAdmin(QVector<PublisherProfileSummary> &outProfiles, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("SELECT username FROM users WHERE role = 'publisher' ORDER BY id ASC");
    if (!query.exec()) {
        errorMsg = "Database error while listing publishers: " + query.lastError().text();
        return false;
    }
    QStringList usernames;
    while (query.next()) {
        usernames.push_back(query.value("username").toString());
    }
    outProfiles.clear();
    for (const QString &username : std::as_const(usernames)) {
        PublisherProfileSummary profile;
        QString profileError;
        if (fetchPublisherProfileForAdmin(username, profile, profileError)) {
            outProfiles.push_back(profile);
        } else {
            qWarning() << "Skipping publisher" << username << "in admin listing:" << profileError;
        }
    }
    return true;
}


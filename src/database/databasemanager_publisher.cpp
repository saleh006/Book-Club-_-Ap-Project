#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>

bool DatabaseManager::fetchPublishedBooks(int publisherId, QVector<Book> &outBooks, QString &errorMsg, bool activeOnly)
{
    QSqlQuery query(m_db);
    query.prepare(activeOnly
                      ? "SELECT * FROM books WHERE publisher_id = :pid AND is_active = 1"
                      : "SELECT * FROM books WHERE publisher_id = :pid");
    query.bindValue(":pid", publisherId);
    if (!query.exec()) {
        errorMsg = "Database error while fetching published books.";
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
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT COALESCE(SUM(pi.price_paid), 0) AS income
        FROM purchase_items pi
        JOIN books b ON b.id = pi.book_id
        WHERE b.publisher_id = :pid
    )");
    query.bindValue(":pid", publisherId);
    if (!query.exec()) {
        errorMsg = "Database error while calculating publisher income.";
        return false;
    }
    outTotalIncome = query.next() ? query.value("income").toDouble() : 0.0;
    return true;
} // i should check this !

bool DatabaseManager::fetchPublisherIncomeForBook(int publisherId, int bookId, double &outIncome, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT COALESCE(SUM(pi.price_paid), 0) AS income
        FROM purchase_items pi
        JOIN books b ON b.id = pi.book_id
        WHERE b.publisher_id = :pid AND b.id = :bid
    )");
    query.bindValue(":pid", publisherId);
    query.bindValue(":bid", bookId);
    if (!query.exec()) {
        errorMsg = "Database error while calculating book income.";
        return false;
    }
    outIncome = query.next() ? query.value("income").toDouble() : 0.0;
    return true;
}

bool DatabaseManager::fetchPublisherStats(int publisherId, int &outBookCount, int &outTotalSales,
                                          double &outAverageRating, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT COUNT(*) AS bookCount,
               COALESCE(SUM(total_sales), 0) AS totalSales,
               COALESCE(AVG(average_rating), 0) AS avgRating
        FROM books
        WHERE publisher_id = :pid AND is_active = 1
    )");
    query.bindValue(":pid", publisherId);
    if (!query.exec()) {
        errorMsg = "Database error while fetching publisher stats.";
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
} // one-shot summary for a publisher dashboard

bool DatabaseManager::setBookOwnership(int bookId, int publisherId, QString &errorMsg)
{
    QSqlQuery query(m_db);
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
#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>

bool DatabaseManager::createShelf(int userId, const QString &title, int &newShelfId, QString &errorMsg) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO shelves (user_id, title) VALUES (:uid, :title)");
    query.bindValue(":uid", userId);
    query.bindValue(":title", title);
    if (!query.exec()) {
        errorMsg = "Failed to create shelf: " + query.lastError().text();
        return false;
    }
    newShelfId = query.lastInsertId().toInt();
    return true;
}

bool DatabaseManager::addBookToShelf(int shelfId, int bookId, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT OR IGNORE INTO shelf_books (shelf_id, book_id) VALUES (:sid, :bid)");
    query.bindValue(":sid", shelfId);
    query.bindValue(":bid", bookId);
    if (!query.exec()) {
        errorMsg = "Failed to add book to shelf: " + query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::fetchShelves(int userId, QVector<Shelf> &outShelves, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT id, title FROM shelves WHERE user_id = :uid");
    query.bindValue(":uid", userId);
    if (!query.exec()) {
        errorMsg = "Database error while fetching shelves.";
        return false;
    }
    outShelves.clear();
    while (query.next()) {
        Shelf s;
        s.id = query.value("id").toInt();
        s.libraryUserId = userId;
        s.title = query.value("title").toString();
        outShelves.push_back(s);
    }
    return true;
}

bool DatabaseManager::fetchShelfBooks(int shelfId, QVector<Book> &outBooks, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT b.* FROM books b
        JOIN shelf_books sb ON sb.book_id = b.id
        WHERE sb.shelf_id = :sid
    )");
    query.bindValue(":sid", shelfId);
    if (!query.exec()) {
        errorMsg = "Database error while fetching shelf books.";
        return false;
    }
    outBooks.clear();
    while (query.next()) {
        Book b;
        b.id = query.value("id").toInt();
        b.title = query.value("title").toString();
        b.author = query.value("author").toString();
        b.coverImagePath = query.value("cover_image_path").toString();
        outBooks.push_back(b);
    }
    return true;
}

bool DatabaseManager::updateReadingProgress(int userId, int bookId, int lastPage, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO reading_progress (user_id, book_id, last_page, updated_at)
        VALUES (:uid, :bid, :page, CURRENT_TIMESTAMP)
        ON CONFLICT(user_id, book_id) DO UPDATE SET
            last_page = :page2, updated_at = CURRENT_TIMESTAMP
    )");
    query.bindValue(":uid", userId);
    query.bindValue(":bid", bookId);
    query.bindValue(":page", lastPage);
    query.bindValue(":page2", lastPage);
    if (!query.exec()) {
        errorMsg = "Failed to update reading progress: " + query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::fetchReadingProgress(int userId, int bookId, ReadingProgress &outProgress, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT last_page FROM reading_progress WHERE user_id = :uid AND book_id = :bid");
    query.bindValue(":uid", userId);
    query.bindValue(":bid", bookId);
    if (!query.exec()) {
        errorMsg = "Database error while fetching reading progress.";
        return false;
    }
    outProgress.userId = userId;
    outProgress.bookId = bookId;
    outProgress.lastPage = query.next() ? query.value("last_page").toInt() : 0;
    return true;
} // returns lastPage 0 if no progress recorded

bool DatabaseManager::fetchOwnedBooks(int userId, QVector<Book> &outBooks, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT DISTINCT b.* FROM books b
        JOIN purchase_items pi ON pi.book_id = b.id
        JOIN purchases p ON p.id = pi.purchase_id
        WHERE p.user_id = :uid
    )");
    query.bindValue(":uid", userId);
    if (!query.exec()) {
        errorMsg = "Database error while fetching owned books.";
        return false;
    }
    outBooks.clear();
    while (query.next()) {
        Book b;
        b.id = query.value("id").toInt();
        b.title = query.value("title").toString();
        b.author = query.value("author").toString();
        b.coverImagePath = query.value("cover_image_path").toString();
        b.pdfPath = query.value("pdf_path").toString();
        outBooks.push_back(b);
    }
    return true;
} //purchased books feeds the NormalUser library

bool DatabaseManager::addToWishlist(int userId, int bookId, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT OR IGNORE INTO wishlist (user_id, book_id) VALUES (:uid, :bid)");
    query.bindValue(":uid", userId);
    query.bindValue(":bid", bookId);
    if (!query.exec()) {
        errorMsg = "Failed to add to wishlist: " + query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::removeFromWishlist(int userId, int bookId, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM wishlist WHERE user_id = :uid AND book_id = :bid");
    query.bindValue(":uid", userId);
    query.bindValue(":bid", bookId);
    if (!query.exec()) {
        errorMsg = "Failed to remove from wishlist: " + query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::fetchWishlist(int userId, QVector<Book> &outBooks, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT b.* FROM books b
        JOIN wishlist w ON w.book_id = b.id
        WHERE w.user_id = :uid
    )");
    query.bindValue(":uid", userId);
    if (!query.exec()) {
        errorMsg = "Database error while fetching wishlist.";
        return false;
    }
    outBooks.clear();
    while (query.next()) {
        Book b;
        b.id = query.value("id").toInt();
        b.title = query.value("title").toString();
        b.author = query.value("author").toString();
        b.price = query.value("price").toDouble();
        b.coverImagePath = query.value("cover_image_path").toString();
        outBooks.push_back(b);
    }
    return true;
}
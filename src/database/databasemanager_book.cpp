#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>


bool DatabaseManager::addBook(const Book &book, int &newBookId, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO books
            (title, author, genre, description, price,
             cover_image_path, pdf_path, is_active)
        VALUES
            (:title, :author, :genre, :description, :price,
             :cover, :pdf, :isActive)
    )");
    query.bindValue(":title", book.title);
    query.bindValue(":author", book.author);
    query.bindValue(":genre", book.genre);
    query.bindValue(":description", book.description);
    query.bindValue(":price", book.price);
    query.bindValue(":cover", book.coverImagePath);
    query.bindValue(":pdf", book.pdfPath);
    query.bindValue(":isActive", book.isActive ? 1 : 0);
    if (!query.exec()) {
        errorMsg = "Failed to add book: " + query.lastError().text();
        return false;
    }
    newBookId = query.lastInsertId().toInt();
    return true;
}

bool DatabaseManager::updateBook(const Book &book, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE books SET
            title = :title, author = :author, genre = :genre,
            description = :description, price = :price,
            cover_image_path = :cover, pdf_path = :pdf,
            is_active = :isActive
        WHERE id = :id
    )");
    query.bindValue(":title", book.title);
    query.bindValue(":author", book.author);
    query.bindValue(":genre", book.genre);
    query.bindValue(":description", book.description);
    query.bindValue(":price", book.price);
    query.bindValue(":cover", book.coverImagePath);
    query.bindValue(":pdf", book.pdfPath);
    query.bindValue(":isActive", book.isActive ? 1 : 0);
    query.bindValue(":id", book.id);
    if (!query.exec()) {
        errorMsg = "Failed to update book: " + query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::deleteBook(int bookId, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE books SET is_active = 0 WHERE id = :id");
    query.bindValue(":id", bookId);
    if (!query.exec()) {
        errorMsg = "Failed to remove book: " + query.lastError().text();
        return false;
    }
    return true;
} // soft delete keeps purchase/review history intact

bool DatabaseManager::fetchBook(int bookId, Book &outBook, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM books WHERE id = :id");
    query.bindValue(":id", bookId);
    if (!query.exec()) {
        errorMsg = "Database error while fetching book.";
        return false;
    }
    if (!query.next()) {
        errorMsg = "Book not found.";
        return false;
    }
    outBook.id = query.value("id").toInt();
    outBook.title = query.value("title").toString();
    outBook.author = query.value("author").toString();
    outBook.genre = query.value("genre").toString();
    outBook.description = query.value("description").toString();
    outBook.price = query.value("price").toDouble();
    outBook.coverImagePath = query.value("cover_image_path").toString();
    outBook.pdfPath = query.value("pdf_path").toString();
    outBook.isActive = query.value("is_active").toBool();
    outBook.averageRating = query.value("average_rating").toDouble();
    outBook.totalSales = query.value("total_sales").toInt();
    return true;
}

bool DatabaseManager::fetchAllBooks(QVector<Book> &outBooks, QString &errorMsg, bool activeOnly)
{
    QSqlQuery query(m_db);
    query.prepare(activeOnly ? "SELECT * FROM books WHERE is_active = 1"
                             : "SELECT * FROM books");
    if (!query.exec()) {
        errorMsg = "Database error while fetching books.";
        return false;
    }
    outBooks.clear();
    while (query.next()) {
        Book b;
        b.id = query.value("id").toInt();
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

bool DatabaseManager::fetchBooksByGenre(const QString &genre, QVector<Book> &outBooks, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM books WHERE genre = :genre AND is_active = 1");
    query.bindValue(":genre", genre);
    if (!query.exec()) {
        errorMsg = "Database error while fetching books by genre.";
        return false;
    }
    outBooks.clear();
    while (query.next()) {
        Book b;
        b.id = query.value("id").toInt();
        b.title = query.value("title").toString();
        b.author = query.value("author").toString();
        b.genre = query.value("genre").toString();
        b.price = query.value("price").toDouble();
        b.coverImagePath = query.value("cover_image_path").toString();
        outBooks.push_back(b);
    }
    return true;
}
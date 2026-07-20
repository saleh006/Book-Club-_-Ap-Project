#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>

bool DatabaseManager::addBook(const Book &book, int &newBookId, QString &errorMsg)
{
    if (book.publisherId <= 0) {
        errorMsg = "A book must belong to a publisher.";
        return false;
    }
    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO books
            (publisher_id, title, author, genre, description, price,
             cover_image_path, pdf_path, is_active)
        VALUES
            (:pubId, :title, :author, :genre, :description, :price,
             :cover, :pdf, :isActive)
    )");
    query.bindValue(":pubId", book.publisherId);
    query.bindValue(":title", book.title);
    query.bindValue(":author", book.author);
    query.bindValue(":genre", book.genre);
    query.bindValue(":description", book.description);
    query.bindValue(":price", book.price);
    query.bindValue(":cover", book.coverImagePath);
    query.bindValue(":pdf", book.pdfPath);
    query.bindValue(":isActive", book.status);

    if (!query.exec()) {
        errorMsg = "Failed to add book: " + query.lastError().text();
        return false;
    }
    newBookId = query.lastInsertId().toInt();
    return true;
}

bool DatabaseManager::updateBook(const Book &book, QString &errorMsg)
{
    if (book.id <= 0) {
        errorMsg = "Invalid book id.";
        return false;
    }
    if (book.publisherId <= 0) {
        errorMsg = "A book must belong to a publisher.";
        return false;
    }

    QSqlQuery query(database());
    query.prepare(R"(
        UPDATE books SET
            title = :title, author = :author, genre = :genre,
            description = :description, price = :price,
            cover_image_path = :cover, pdf_path = :pdf,
            is_active = :isActive
        WHERE id = :id AND publisher_id = :pubId
    )");
    query.bindValue(":title", book.title);
    query.bindValue(":author", book.author);
    query.bindValue(":genre", book.genre);
    query.bindValue(":description", book.description);
    query.bindValue(":price", book.price);
    query.bindValue(":cover", book.coverImagePath);
    query.bindValue(":pdf", book.pdfPath);
    query.bindValue(":isActive", book.status);
    query.bindValue(":id", book.id);
    query.bindValue(":pubId", book.publisherId);

    if (!query.exec()) {
        errorMsg = "Failed to update book: " + query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        errorMsg = "Book not found or you don't have permission to edit it.";
        return false;
    }
    return true;
}

bool DatabaseManager::deleteBook(int bookId, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("UPDATE books SET is_active = -1 WHERE id = :id");
    query.bindValue(":id", bookId);

    if (!query.exec()) {
        errorMsg = "Failed to remove book: " + query.lastError().text();
        return false;
    }
    return true;
} // soft delete keeps purchase/review history intact

bool DatabaseManager::fetchBook(int bookId, Book &outBook, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("SELECT * FROM books WHERE id = :id");
    query.bindValue(":id", bookId);

    if (!query.exec()) {
        errorMsg = "Database error while fetching book: " + query.lastError().text();
        return false;
    }
    if (!query.next()) {
        errorMsg = "Book not found.";
        return false;
    }

    outBook.id = query.value("id").toInt();
    outBook.publisherId = query.value("publisher_id").toInt();
    outBook.title = query.value("title").toString();
    outBook.author = query.value("author").toString();
    outBook.genre = query.value("genre").toString();
    outBook.description = query.value("description").toString();
    outBook.price = query.value("price").toDouble();
    outBook.coverImagePath = query.value("cover_image_path").toString();
    outBook.pdfPath = query.value("pdf_path").toString();
    outBook.status = query.value("is_active").toInt();
    outBook.averageRating = query.value("average_rating").toDouble();
    outBook.totalSales = query.value("total_sales").toInt();
    return true;
}

bool DatabaseManager::fetchAllBooks(QVector<Book> &outBooks, QString &errorMsg, bool activeOnly)
{
    QSqlQuery query(database());
    query.prepare(activeOnly
                      ? "SELECT b.*, u.full_name AS publisher_name FROM books b "
                        "LEFT JOIN users u ON u.id = b.publisher_id WHERE b.is_active = 1"
                      : "SELECT b.*, u.full_name AS publisher_name FROM books b "
                        "LEFT JOIN users u ON u.id = b.publisher_id");

    if (!query.exec()) {
        errorMsg = "Database error while fetching books: " + query.lastError().text();
        return false;
    }

    outBooks.clear();
    while (query.next()) {
        Book b;
        b.id = query.value("id").toInt();
        b.publisherId = query.value("publisher_id").toInt();
        b.title = query.value("title").toString();
        b.author = query.value("author").toString();
        b.genre = query.value("genre").toString();
        b.description = query.value("description").toString();
        b.price = query.value("price").toDouble();
        b.coverImagePath = query.value("cover_image_path").toString();
        b.pdfPath = query.value("pdf_path").toString();
        b.status = query.value("is_active").toInt();
        b.averageRating = query.value("average_rating").toDouble();
        b.totalSales = query.value("total_sales").toInt();
        b.publisherName = query.value("publisher_name").toString();
        outBooks.push_back(b);
    }
    return true;
}

bool DatabaseManager::fetchBooksByGenre(const QString &genre, QVector<Book> &outBooks, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("SELECT * FROM books WHERE genre = :genre AND is_active = 1");
    query.bindValue(":genre", genre);

    if (!query.exec()) {
        errorMsg = "Database error while fetching books by genre: " + query.lastError().text();
        return false;
    }

    outBooks.clear();
    while (query.next()) {
        Book b;
        b.id = query.value("id").toInt();
        b.publisherId = query.value("publisher_id").toInt();
        b.title = query.value("title").toString();
        b.author = query.value("author").toString();
        b.genre = query.value("genre").toString();
        b.description = query.value("description").toString();
        b.price = query.value("price").toDouble();
        b.coverImagePath = query.value("cover_image_path").toString();
        b.pdfPath = query.value("pdf_path").toString();
        b.status = query.value("is_active").toInt();
        b.averageRating = query.value("average_rating").toDouble();
        b.totalSales = query.value("total_sales").toInt();
        outBooks.push_back(b);
    }
    return true;
}

bool DatabaseManager::setBookStatus(int bookId, int status, QString &errorMsg)
{
    if (status != -1 && status != 0 && status != 1) {
        errorMsg = "Invalid book status value.";
        return false;
    }
    QSqlQuery query(database());
    query.prepare("UPDATE books SET is_active = :status WHERE id = :id");
    query.bindValue(":status", status);
    query.bindValue(":id", bookId);

    if (!query.exec()) {
        errorMsg = "Failed to update book status: " + query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        errorMsg = "Book not found.";
        return false;
    }
    return true;
}
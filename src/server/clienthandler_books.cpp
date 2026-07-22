#include <QJsonArray>
#include "databasemanager.h"
#include "clienthandler.h"

bool ClientHandler::handleBookActions(const QString &action, const QJsonObject &requestObj, QJsonObject &responseObj)
{
    if (action == "books_add") {
        Book b;
        b.publisherId = requestObj["publisherId"].toInt();
        b.title = requestObj["title"].toString();
        b.author = requestObj["author"].toString();
        b.genre = requestObj["genre"].toString();
        b.description = requestObj["description"].toString();
        b.price = requestObj["price"].toDouble();
        b.coverImagePath = requestObj["coverImagePath"].toString();
        b.pdfPath = requestObj["pdfPath"].toString();

        if (b.price < 0) {
            responseObj["status"] = "error";
            responseObj["message"] = "Book price cannot be negative.";
        } else {
            int newBookId = -1;
            QString errorMsg;
            if (DatabaseManager::instance().addBook(b, newBookId, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["message"] = "Book added successfully.";
                responseObj["bookId"] = newBookId;
                emit databaseUpdated("book");
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
    }
    else if (action == "book_update") {
        responseObj["action"] = "book_update_response";
        Book b;
        b.id = requestObj["id"].toInt();
        b.title = requestObj["title"].toString();
        b.author = requestObj["author"].toString();
        b.genre = requestObj["genre"].toString();
        b.description = requestObj["description"].toString();
        b.price = requestObj["price"].toDouble();
        b.coverImagePath = requestObj["coverImagePath"].toString();
        b.pdfPath = requestObj["pdfPath"].toString();

        QString errorMsg;
        if (DatabaseManager::instance().updateBook(b, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Book information updated successfully.";
            emit databaseUpdated("book");
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "get_book_details") {
        responseObj["action"] = "book_details_response";
        int bookId = requestObj["bookId"].toInt();
        Book b;
        QString errorMsg;

        if (DatabaseManager::instance().fetchBook(bookId, b, errorMsg)) {
            responseObj["status"] = "success";
            QJsonObject data;
            data["id"] = b.id;
            data["title"] = b.title;
            data["author"] = b.author;
            data["genre"] = b.genre;
            data["description"] = b.description;
            data["price"] = b.price;
            data["coverImagePath"] = b.coverImagePath;
            data["pdfPath"] = b.pdfPath;
            data["averageRating"] = b.averageRating;
            data["isActive"] = b.status;
            responseObj["data"] = data;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg.isEmpty() ? "Book not found." : errorMsg;
        }
    }
    else if (action == "get_books_list") {
        responseObj["action"] = "books_list_response";
        QVector<Book> books;
        QString errorMsg;

        if (DatabaseManager::instance().fetchAllBooks(books, errorMsg, false)) {
            QJsonArray booksArray;
            for (const auto &book : books) {
                QJsonObject bookObj;
                bookObj["id"] = book.id;
                bookObj["title"] = book.title;
                bookObj["author"] = book.author;
                bookObj["isActive"] = book.status;
                bookObj["price"] = book.price;

                booksArray.append(bookObj);
            }

            responseObj["status"] = "success";
            responseObj["data"] = booksArray;
        } else {
            QJsonObject response;
            response["status"] = "error";
            response["message"] = errorMsg;
            sendToClient(response);
        }
    }
    else if (action == "books_fetch_all") {
        responseObj["action"] = "books_fetch_all_response";
        QVector<Book> books;
        QString errorMsg;
        bool activeOnly = requestObj["activeOnly"].toBool(true);
        if (DatabaseManager::instance().fetchAllBooks(books, errorMsg, activeOnly)) {
            responseObj["status"] = "success";
            QJsonArray bookArray;
            for (const Book &b : books) {
                QJsonObject bookObj;
                bookObj["id"] = b.id;
                bookObj["title"] = b.title;
                bookObj["author"] = b.author;
                bookObj["genre"] = b.genre;
                bookObj["price"] = b.price;
                bookObj["averageRating"] = b.averageRating;
                bookObj["description"] = b.description;        // <-- add
                bookObj["coverImagePath"] = b.coverImagePath;  // <-- add
                bookObj["totalSales"] = b.totalSales;
                bookArray.append(bookObj);
            }
            responseObj["books"] = bookArray;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "books_fetch_by_genre") {
        QString genre = requestObj["genre"].toString();
        QVector<Book> books;
        QString errorMsg;
        if (DatabaseManager::instance().fetchBooksByGenre(genre, books, errorMsg)) {
            responseObj["status"] = "success";
            QJsonArray bookArray;
            for (const Book &b : books) {
                QJsonObject bookObj;
                bookObj["id"] = b.id;
                bookObj["title"] = b.title;
                bookObj["author"] = b.author;
                bookObj["price"] = b.price;
                bookObj["averageRating"] = b.averageRating;
                bookArray.append(bookObj);
            }
            responseObj["books"] = bookArray;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else {
        return false;
    }
    return true;
}

#include <QJsonArray>
#include "databasemanager.h"
#include "clienthandler.h"

bool ClientHandler::handleDiscount_Wishlist_ReviewsActions(const QString &action, const QJsonObject &requestObj, QJsonObject &responseObj)
{
    if (action == "discount_add") {
        Discount d;
        d.bookId = requestObj["bookId"].toInt();
        d.type = requestObj["type"].toString();
        d.value = requestObj["value"].toDouble();
        QString errorMsg;
        if (DatabaseManager::instance().addDiscount(d, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "New discount added successfully.";
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "discount_fetch_active") {
        int bookId = requestObj["bookId"].toInt();
        Discount d;
        QString errorMsg;
        if (DatabaseManager::instance().fetchActiveDiscount(bookId, d, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["type"] = d.type;
            responseObj["value"] = d.value;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "wishlist_add") {
        responseObj["action"] = "wishlist_add_response";
        int userId = requestObj["userId"].toInt();
        int bookId = requestObj["bookId"].toInt();
        QString errorMsg;
        if (DatabaseManager::instance().addToWishlist(userId, bookId, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Added to wishlist.";
            responseObj["bookId"] = bookId;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "wishlist_remove") {
        responseObj["action"] = "wishlist_remove_response";
        int userId = requestObj["userId"].toInt();
        int bookId = requestObj["bookId"].toInt();
        QString errorMsg;
        if (DatabaseManager::instance().removeFromWishlist(userId, bookId, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Removed from wishlist.";
            responseObj["bookId"] = bookId;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "wishlist_fetch") {
        responseObj["action"] = "wishlist_fetch_response";
        int userId = requestObj["userId"].toInt();
        QVector<Book> books;
        QString errorMsg;
        if (DatabaseManager::instance().fetchWishlist(userId, books, errorMsg)) {
            responseObj["status"] = "success";
            QJsonArray bookArray;
            for (const Book &b : books) {
                QJsonObject bObj;
                bObj["id"] = b.id;
                bObj["title"] = b.title;
                bObj["author"] = b.author;
                bookArray.append(bObj);
            }
            responseObj["books"] = bookArray;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "shelf_create") {
        int userId = requestObj["userId"].toInt();
        QString title = requestObj["title"].toString();
        int newShelfId = -1;
        QString errorMsg;
        if (DatabaseManager::instance().createShelf(userId, title, newShelfId, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "New shelf created successfully.";
            responseObj["shelfId"] = newShelfId;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "shelf_add_book") {
        int shelfId = requestObj["shelfId"].toInt();
        int bookId = requestObj["bookId"].toInt();
        QString errorMsg;
        if (DatabaseManager::instance().addBookToShelf(shelfId, bookId, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Book added to shelf.";
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "shelves_fetch") {
        int userId = requestObj["userId"].toInt();
        QVector<Shelf> shelves;
        QString errorMsg;
        if (DatabaseManager::instance().fetchShelves(userId, shelves, errorMsg)) {
            responseObj["status"] = "success";
            QJsonArray shelfArray;
            for (const Shelf &s : shelves) {
                QJsonObject sObj;
                sObj["id"] = s.id;
                sObj["title"] = s.title;
                shelfArray.append(sObj);
            }
            responseObj["shelves"] = shelfArray;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "shelf_fetch_books") {
        int shelfId = requestObj["shelfId"].toInt();
        QVector<Book> books;
        QString errorMsg;
        if (DatabaseManager::instance().fetchShelfBooks(shelfId, books, errorMsg)) {
            responseObj["status"] = "success";
            QJsonArray bookArray;
            for (const Book &b : books) {
                QJsonObject bObj;
                bObj["id"] = b.id;
                bObj["title"] = b.title;
                bObj["author"] = b.author;
                bookArray.append(bObj);
            }
            responseObj["books"] = bookArray;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "progress_update") {
        int userId = requestObj["userId"].toInt();
        int bookId = requestObj["bookId"].toInt();
        int lastPage = requestObj["lastPage"].toInt();
        QString errorMsg;
        if (DatabaseManager::instance().updateReadingProgress(userId, bookId, lastPage, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Reading progress updated successfully.";
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "progress_fetch") {
        int userId = requestObj["userId"].toInt();
        int bookId = requestObj["bookId"].toInt();
        ReadingProgress p;
        QString errorMsg;
        if (DatabaseManager::instance().fetchReadingProgress(userId, bookId, p, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["lastPage"] = p.lastPage;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "books_fetch_owned") {
        int userId = requestObj["userId"].toInt();
        QVector<Book> books;
        QString errorMsg;
        if (DatabaseManager::instance().fetchOwnedBooks(userId, books, errorMsg)) {
            responseObj["status"] = "success";
            QJsonArray bookArray;
            for (const Book &b : books) {
                QJsonObject bObj;
                bObj["id"] = b.id;
                bObj["title"] = b.title;
                bObj["author"] = b.author;
                bObj["pdfPath"] = b.pdfPath;
                bookArray.append(bObj);
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

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

            Book b;
            QString bookErr;
            if (DatabaseManager::instance().fetchBook(d.bookId, b, bookErr)) {
                QVector<int> wl;
                QString wErr;
                DatabaseManager::instance().fetchUserIdsWithBookInWishlist(d.bookId, wl, wErr);
                for (int uid : wl) {
                    notifyUser(uid, "Price Drop!", QString("\"%1\" now has a limited-time discount.").arg(b.title));
                }
            }
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
    else if (action == "publisher_get_discounts") {
        responseObj["type"] = "publisher_discounts_list";
        int publisherId = requestObj["publisherId"].toInt();

        QVector<DiscountPublisherSummary> discounts;
        QString errorMsg;
        if (DatabaseManager::instance().fetchDiscountsForPublisher(publisherId, discounts, errorMsg)) {
            responseObj["status"] = "success";
            QJsonArray arr;
            for (const DiscountPublisherSummary &s : discounts) {
                QJsonObject dObj;
                dObj["discountId"] = s.discount.id;
                dObj["bookId"] = s.discount.bookId;
                dObj["bookTitle"] = s.bookTitle;
                dObj["type"] = s.discount.type;
                dObj["value"] = s.discount.value;
                dObj["startDate"] = s.discount.startDate.toString(Qt::ISODate);
                dObj["endDate"] = s.discount.endDate.toString(Qt::ISODate);
                arr.append(dObj);
            }
            responseObj["discounts"] = arr;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "publisher_update_discount") {
        responseObj["type"] = "publisher_discount_action_result";
        int publisherId = requestObj["publisherId"].toInt();
        int discountId = requestObj["discountId"].toInt();

        Discount existing;
        QString errorMsg;
        if (!DatabaseManager::instance().fetchDiscount(discountId, existing, errorMsg)) {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        } else {
            Book b;
            QString bookErr;
            if (!DatabaseManager::instance().fetchBook(existing.bookId, b, bookErr) || b.publisherId != publisherId) {
                responseObj["status"] = "error";
                responseObj["message"] = "You do not have permission to edit this discount.";
            } else {
                Discount d = existing;
                if (requestObj.contains("value"))
                    d.value = requestObj["value"].toDouble();
                if (requestObj.contains("type"))
                    d.type = requestObj["type"].toString();
                if (requestObj.contains("startDate"))
                    d.startDate = QDateTime::fromString(requestObj["startDate"].toString(), Qt::ISODate);
                if (requestObj.contains("endDate"))
                    d.endDate = QDateTime::fromString(requestObj["endDate"].toString(), Qt::ISODate);

                if (DatabaseManager::instance().updateDiscount(d, errorMsg)) {
                    responseObj["status"] = "success";
                    responseObj["message"] = "Discount updated successfully.";
                } else {
                    responseObj["status"] = "error";
                    responseObj["message"] = errorMsg;
                }
            }
        }
    }
    else if (action == "publisher_delete_discount") {
        responseObj["type"] = "publisher_discount_action_result";
        int publisherId = requestObj["publisherId"].toInt();
        int discountId = requestObj["discountId"].toInt();

        Discount existing;
        QString errorMsg;
        if (!DatabaseManager::instance().fetchDiscount(discountId, existing, errorMsg)) {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        } else {
            Book b;
            QString bookErr;
            if (!DatabaseManager::instance().fetchBook(existing.bookId, b, bookErr) || b.publisherId != publisherId) {
                responseObj["status"] = "error";
                responseObj["message"] = "You do not have permission to delete this discount.";
            } else if (DatabaseManager::instance().deleteDiscount(discountId, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["message"] = "Discount deleted successfully.";
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
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
                bObj["status"] = b.status;
                bookArray.append(bObj);
            }
            responseObj["books"] = bookArray;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "shelf_create") {
        responseObj["action"] = "shelf_create_response";
        int userId = requestObj["userId"].toInt();
        QString title = requestObj["title"].toString();
        int newShelfId = -1;
        QString errorMsg;
        if (DatabaseManager::instance().createShelf(userId, title, newShelfId, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "New shelf created successfully.";
            responseObj["shelfId"] = newShelfId;
            responseObj["title"] = title;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "shelf_add_book") {
        responseObj["action"] = "shelf_add_book_response";
        int shelfId = requestObj["shelfId"].toInt();
        int bookId = requestObj["bookId"].toInt();
        QString errorMsg;
        if (DatabaseManager::instance().addBookToShelf(shelfId, bookId, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Book added to shelf.";
            responseObj["shelfId"] = shelfId;
            responseObj["bookId"] = bookId;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "shelves_fetch") {
        responseObj["action"] = "shelves_fetch_response";
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
        responseObj["action"] = "shelf_fetch_books_response";
        int shelfId = requestObj["shelfId"].toInt();
        responseObj["shelfId"] = shelfId;
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
        responseObj["action"] = "progress_update_response";
        int userId = requestObj["userId"].toInt();
        int bookId = requestObj["bookId"].toInt();
        int lastPage = requestObj["lastPage"].toInt();
        responseObj["bookId"] = bookId;
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
        responseObj["action"] = "progress_fetch_response";
        int userId = requestObj["userId"].toInt();
        int bookId = requestObj["bookId"].toInt();
        responseObj["bookId"] = bookId;
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
        responseObj["action"] = "books_fetch_owned_response";
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
    else if (action == "shelf_remove_book") {
        responseObj["action"] = "shelf_remove_book_response";
        int shelfId = requestObj["shelfId"].toInt();
        int bookId = requestObj["bookId"].toInt();
        responseObj["shelfId"] = shelfId;
        responseObj["bookId"] = bookId;
        QString errorMsg;
        if (DatabaseManager::instance().removeBookFromShelf(shelfId, bookId, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Book removed from shelf.";
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "shelf_update") {
        responseObj["action"] = "shelf_update_response";
        int shelfId = requestObj["shelfId"].toInt();
        QString newTitle = requestObj["title"].toString();
        responseObj["shelfId"] = shelfId;
        QString errorMsg;
        if (DatabaseManager::instance().updateShelf(shelfId, newTitle, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Shelf updated successfully.";
            responseObj["title"] = newTitle;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "shelf_delete") {
        responseObj["action"] = "shelf_delete_response";
        int shelfId = requestObj["shelfId"].toInt();
        responseObj["shelfId"] = shelfId;
        QString errorMsg;
        if (DatabaseManager::instance().deleteShelf(shelfId, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Shelf deleted successfully.";
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "progress_update") {
        responseObj["action"] = "progress_update_response";
        int userId = requestObj["userId"].toInt();
        int bookId = requestObj["bookId"].toInt();
        int lastPage = requestObj["lastPage"].toInt();
        responseObj["bookId"] = bookId;
        QString errorMsg;
        if (DatabaseManager::instance().updateReadingProgress(userId, bookId, lastPage, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Reading progress updated successfully.";
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "progress_fetch_all") {
        responseObj["action"] = "progress_fetch_all_response";
        int userId = requestObj["userId"].toInt();
        QVector<ReadingProgress> progressList;
        QString errorMsg;
        if (DatabaseManager::instance().fetchAllReadingProgress(userId, progressList, errorMsg)) {
            responseObj["status"] = "success";
            QJsonArray arr;
            for (const ReadingProgress &p : progressList) {
                QJsonObject po;
                po["bookId"] = p.bookId;
                po["lastPage"] = p.lastPage;
                arr.append(po);
            }
            responseObj["progress"] = arr;
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
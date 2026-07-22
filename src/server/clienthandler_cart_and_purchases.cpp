#include <QJsonArray>
#include <QDateTime>
#include "databasemanager.h"
#include "clienthandler.h"

bool ClientHandler::handleCart_PurchaseActions(const QString &action, const QJsonObject &requestObj, QJsonObject &responseObj)
{
    if (action == "add_to_cart") {
        responseObj["action"] = "add_to_cart_response";
        int userId = requestObj["userId"].toInt();
        int bookId = requestObj["bookId"].toInt();
        int quantity = requestObj["quantity"].toInt(1);

        if (quantity < 1) {
            responseObj["status"] = "error";
            responseObj["message"] = "Quantity must be at least 1.";
        } else {
            QString errorMsg;
            if (DatabaseManager::instance().addToCart(userId, bookId, quantity, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["message"] = "Book successfully added to cart.";
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
    }
    else if (action == "remove_from_cart") {
        responseObj["action"] = "remove_from_cart_response";
        int userId = requestObj["userId"].toInt();
        int bookId = requestObj["bookId"].toInt();
        QString errorMsg;
        if (DatabaseManager::instance().removeFromCart(userId, bookId, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Book removed from cart.";
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "cart_clear") {
        responseObj["action"] = "cart_clear_response";
        int userId = requestObj["userId"].toInt();
        QString errorMsg;
        if (DatabaseManager::instance().clearCart(userId, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Cart cleared successfully.";
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "cart_fetch") {
        responseObj["action"] = "cart_fetch_response";
        int userId = requestObj["userId"].toInt();
        QVector<CartItem> items;
        double totalPrice = 0.0;
        QString errorMsg;
        if (DatabaseManager::instance().fetchCart(userId, items, totalPrice, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["totalPrice"] = totalPrice;
            QJsonArray cartArray;
            for (const CartItem &item : items) {
                QJsonObject itemObj;
                itemObj["bookId"] = item.bookId;
                itemObj["quantity"] = item.quantity;
                itemObj["price"] = item.price;
                Book book;
                QString bookErr;
                if (DatabaseManager::instance().fetchBook(item.bookId, book, bookErr)) {
                    itemObj["title"] = book.title;
                    itemObj["author"] = book.author;
                    itemObj["coverImagePath"] = book.coverImagePath;
                    itemObj["originalPrice"] = book.price;
                    itemObj["genre"] = book.genre;

                    QVector<Book> wishlist;
                    QString wishErr;
                    bool inWishlist = false;
                    if (DatabaseManager::instance().fetchWishlist(userId, wishlist, wishErr)) {
                        for (const Book &w : wishlist) {
                            if (w.id == item.bookId) { inWishlist = true; break; }
                        }
                    }
                    itemObj["favorite"] = inWishlist;
                }
                cartArray.append(itemObj);
            }
            responseObj["items"] = cartArray;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "checkout") {
        responseObj["action"] = "checkout_response";
        int userId = requestObj["userId"].toInt();
        QString errorMsg;
        int purchaseId = -1;
        if (DatabaseManager::instance().checkoutCart(userId, errorMsg, purchaseId)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Purchase completed successfully and cart cleared.";
            responseObj["purchaseId"] = purchaseId;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "purchase_fetch_history") {
        responseObj["action"] = "purchase_fetch_history_response";
        int userId = requestObj["userId"].toInt();
        QVector<Purchase> purchases;
        QString errorMsg;
        if (DatabaseManager::instance().fetchPurchaseHistory(userId, purchases, errorMsg)) {
            responseObj["status"] = "success";
            QJsonArray purchaseArray;
            for (const Purchase &p : purchases) {
                QJsonObject pObj;
                pObj["id"] = p.id;
                pObj["totalPrice"] = p.totalPrice;
                pObj["date"] = p.purchaseDate.toString(Qt::ISODate);
                purchaseArray.append(pObj);
            }
            responseObj["purchases"] = purchaseArray;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    /*else if (action == "purchase_fetch_details") {
            int purchaseId = requestObj["purchaseId"].toInt();
            QVector<CartItem> details;
            QString errorMsg;
            if (DatabaseManager::instance().fetchPurchaseDetails(purchaseId, details, errorMsg)) {
                responseObj["status"] = "success";
                QJsonArray detailsArray;
                for (const CartItem &item : details) {
                    QJsonObject itemObj;
                    itemObj["bookId"] = item.bookId;
                    itemObj["quantity"] = item.quantity;
                    itemObj["price"] = item.price;
                    detailsArray.append(itemObj);
                }
                responseObj["details"] = detailsArray;
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }*/
    else {
        return false;
    }
    return true;
}

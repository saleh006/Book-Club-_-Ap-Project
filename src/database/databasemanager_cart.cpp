#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>
#include <QtGlobal>

bool DatabaseManager::addToCart(int userId, int bookId, int quantity, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO cart_items (user_id, book_id, quantity)
        VALUES (:uid, :bid, :qty)
        ON CONFLICT(user_id, book_id) DO UPDATE SET quantity = quantity + :qty2
    )");
    query.bindValue(":uid", userId);
    query.bindValue(":bid", bookId);
    query.bindValue(":qty", quantity);
    query.bindValue(":qty2", quantity);
    if (!query.exec()) {
        errorMsg = "Failed to add to cart: " + query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::removeFromCart(int userId, int bookId, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM cart_items WHERE user_id = :uid AND book_id = :bid");
    query.bindValue(":uid", userId);
    query.bindValue(":bid", bookId);
    if (!query.exec()) {
        errorMsg = "Failed to remove from cart: " + query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::clearCart(int userId, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM cart_items WHERE user_id = :uid");
    query.bindValue(":uid", userId);
    if (!query.exec()) {
        errorMsg = "Failed to clear cart: " + query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::fetchCart(int userId, QVector<CartItem> &outItems, double &totalPrice, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT c.book_id, c.quantity, b.price FROM cart_items c
        JOIN books b ON b.id = c.book_id
        WHERE c.user_id = :uid
    )");
    query.bindValue(":uid", userId);
    if (!query.exec()) {
        errorMsg = "Database error while fetching cart.";
        return false;
    }
    outItems.clear();
    totalPrice = 0.0;
    while (query.next()) {
        CartItem item;
        item.bookId = query.value("book_id").toInt();
        item.quantity = query.value("quantity").toInt();
        double basePrice = query.value("price").toDouble();

        Discount discount;
        QString discErr;
        double effectivePrice = basePrice;
        if (fetchActiveDiscount(item.bookId, discount, discErr) && discount.id != -1) {
            if (discount.type == "percent")
                effectivePrice = basePrice * (1.0 - discount.value / 100.0);
            else if (discount.type == "flat")
                effectivePrice = qMax(0.0, basePrice - discount.value);
        }
        item.price = effectivePrice;

        totalPrice += item.price * item.quantity;
        outItems.push_back(item);
    }
    return true;
}

bool DatabaseManager::checkoutCart(int userId, QString &errorMsg, int &purchaseId)
{
    QVector<CartItem> items;
    double total = 0.0;
    if (!fetchCart(userId, items, total, errorMsg))
        return false;
    if (items.isEmpty()) {
        errorMsg = "Your cart is empty.";
        return false;
    }

    if (!m_db.transaction()) {
        errorMsg = "Failed to start transaction.";
        return false;
    }

    QSqlQuery insertPurchase(m_db);
    insertPurchase.prepare(R"(
        INSERT INTO purchases (user_id, total_price, purchase_date)
        VALUES (:uid, :total, :date)
    )");
    insertPurchase.bindValue(":uid", userId);
    insertPurchase.bindValue(":total", total);
    insertPurchase.bindValue(":date", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!insertPurchase.exec()) {
        errorMsg = "Failed to record purchase: " + insertPurchase.lastError().text();
        m_db.rollback();
        return false;
    }
    purchaseId = insertPurchase.lastInsertId().toInt();

    for (const CartItem &item : std::as_const(items)) {
        QSqlQuery insertItem(m_db);
        insertItem.prepare(R"(
            INSERT INTO purchase_items (purchase_id, book_id, quantity, price_paid)
            VALUES (:pid, :bid, :qty, :price)
        )");
        insertItem.bindValue(":pid", purchaseId);
        insertItem.bindValue(":bid", item.bookId);
        insertItem.bindValue(":qty", item.quantity);
        insertItem.bindValue(":price", item.price);   // effective (discounted) price, not base price
        if (!insertItem.exec()) {
            errorMsg = "Failed to record purchased item: " + insertItem.lastError().text();
            m_db.rollback();
            return false;
        }

        QSqlQuery bumpSales(m_db);
        bumpSales.prepare("UPDATE books SET total_sales = total_sales + :qty WHERE id = :bid");
        bumpSales.bindValue(":qty", item.quantity);
        bumpSales.bindValue(":bid", item.bookId);
        if (!bumpSales.exec()) {
            errorMsg = "Failed to update book sales: " + bumpSales.lastError().text();
            m_db.rollback();
            return false;
        }
    }

    QSqlQuery clearCartQuery(m_db);
    clearCartQuery.prepare("DELETE FROM cart_items WHERE user_id = :uid");
    clearCartQuery.bindValue(":uid", userId);
    if (!clearCartQuery.exec()) {
        errorMsg = "Failed to clear cart: " + clearCartQuery.lastError().text();
        m_db.rollback();
        return false;
    }

    if (!m_db.commit()) {
        errorMsg = "Failed to finalize purchase.";
        m_db.rollback();
        return false;
    }
    return true;
}

bool DatabaseManager::fetchPurchaseHistory(int userId, QVector<Purchase> &outPurchases, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT id, total_price, purchase_date FROM purchases WHERE user_id = :uid ORDER BY purchase_date DESC");
    query.bindValue(":uid", userId);
    if (!query.exec()) {
        errorMsg = "Database error while fetching purchase history.";
        return false;
    }
    outPurchases.clear();
    while (query.next()) {
        Purchase p;
        p.id = query.value("id").toInt();
        p.userId = userId;
        p.totalPrice = query.value("total_price").toDouble();

        QDateTime date = QDateTime::fromString(query.value("purchase_date").toString(), Qt::ISODate);
        date.toUTC();
        p.purchaseDate = date;   // caller does .toLocalTime() when displaying

        QSqlQuery itemsQuery(m_db);
        itemsQuery.prepare("SELECT book_id, quantity, price_paid FROM purchase_items WHERE purchase_id = :pid");
        itemsQuery.bindValue(":pid", p.id);
        if (itemsQuery.exec()) {
            while (itemsQuery.next())
                p.bookIds.push_back(itemsQuery.value("book_id").toInt());
        }
        outPurchases.push_back(p);
    }
    return true;
}
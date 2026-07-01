#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>

bool DatabaseManager::addDiscount(const Discount &discount, QString &errorMsg)
{
    if (discount.startDate >= discount.endDate) {
        errorMsg = "Discount start time must be before end time.";
        return false;
    }
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO discounts (book_id, type, value, start_date, end_date)
        VALUES (:bid, :type, :value, :start, :end)
    )");
    query.bindValue(":bid", discount.bookId);
    query.bindValue(":type", discount.type);
    query.bindValue(":value", discount.value);
    query.bindValue(":start", discount.startDate.toUTC().toString(Qt::ISODate));
    query.bindValue(":end", discount.endDate.toUTC().toString(Qt::ISODate));
    if (!query.exec()) {
        errorMsg = "Failed to add discount: " + query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::fetchActiveDiscount(int bookId,
                                          Discount &outDiscount,
                                          QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, type, value, start_date, end_date
        FROM discounts
        WHERE book_id = :bid
        ORDER BY id DESC
        LIMIT 1
    )");
    query.bindValue(":bid", bookId);
    if (!query.exec()) {
        errorMsg = "Database error while fetching discount.";
        return false;
    }
    if (!query.next()) {
        outDiscount = Discount{};
        return true;
    }
    QDateTime start = QDateTime::fromString(query.value("start_date").toString(), Qt::ISODate);
    start.toUTC();
    QDateTime end = QDateTime::fromString(query.value("end_date").toString(), Qt::ISODate);
    end.toUTC();
    QDateTime now = QDateTime::currentDateTimeUtc();
    if (now < start || now > end) {
        outDiscount = Discount{};
        return true;
    }
    outDiscount.id = query.value("id").toInt();
    outDiscount.bookId = bookId;
    outDiscount.type = query.value("type").toString();
    outDiscount.value = query.value("value").toDouble();
    outDiscount.startDate = start;
    outDiscount.endDate = end;
    return true;
}


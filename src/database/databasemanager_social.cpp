#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>
#include <QDateTime>

bool DatabaseManager::addDiscount(const Discount &discount, QString &errorMsg)
{
    if (discount.startDate >= discount.endDate) {
        errorMsg = "Discount start time must be before end time.";
        return false;
    }
    QSqlQuery query(database());
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
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT id, type, value, start_date, end_date
        FROM discounts
        WHERE book_id = :bid
        ORDER BY id DESC
        LIMIT 1
    )");
    query.bindValue(":bid", bookId);
    if (!query.exec()) {
        errorMsg = "Database error while fetching discount: " + query.lastError().text();
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

bool DatabaseManager::addReview(const Review &review, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO reviews (user_id, book_id, comment, rating)
        VALUES (:uid, :bid, :comment, :rating)
    )");
    query.bindValue(":uid", review.userId);
    query.bindValue(":bid", review.bookId);
    query.bindValue(":comment", review.comment);
    query.bindValue(":rating", review.rating);
    if (!query.exec()) {
        errorMsg = "Failed to add review: " + query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::fetchReviewsForBook(int bookId, QVector<Review> &outReviews, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("SELECT id, user_id, comment, rating, date, is_approved FROM reviews WHERE book_id = :bid AND is_approved = 1 ORDER BY date DESC");
    query.bindValue(":bid", bookId);
    if (!query.exec()) {
        errorMsg = "Database error while fetching reviews: " + query.lastError().text();
        return false;
    }
    outReviews.clear();
    while (query.next()) {
        Review r;
        r.id = query.value("id").toInt();
        r.userId = query.value("user_id").toInt();
        r.bookId = bookId;
        r.comment = query.value("comment").toString();
        r.rating = query.value("rating").toInt();
        r.date = query.value("date").toDateTime();
        r.isApproved = query.value("is_approved").toBool();
        outReviews.push_back(r);
    }
    return true;
}

bool DatabaseManager::recalculateAverageRating(int bookId, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("UPDATE books SET average_rating = (SELECT COALESCE(AVG(rating), 0) FROM reviews WHERE book_id = :bid AND is_approved = 1) WHERE id = :bid2");
    query.bindValue(":bid", bookId);
    query.bindValue(":bid2", bookId);
    if (!query.exec()) {
        errorMsg = "Failed to update average rating: " + query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::fetchReviewsForAdmin(bool pendingOnly, QVector<ReviewAdminSummary> &outReviews, QString &errorMsg)
{
    QSqlQuery query(database());
    QString sql =
        "SELECT r.id, r.user_id, r.book_id, r.comment, r.rating, r.date, r.is_approved, "
        "b.title AS book_title, u.username AS username "
        "FROM reviews r "
        "JOIN books b ON r.book_id = b.id "
        "JOIN users u ON r.user_id = u.id ";
    if (pendingOnly) {
        sql += "WHERE r.is_approved = 0 ";
    }
    sql += "ORDER BY r.date DESC";

    query.prepare(sql);
    if (!query.exec()) {
        errorMsg = "Database error while fetching reviews: " + query.lastError().text();
        return false;
    }

    outReviews.clear();
    while (query.next()) {
        ReviewAdminSummary summary;
        summary.review.id = query.value("id").toInt();
        summary.review.userId = query.value("user_id").toInt();
        summary.review.bookId = query.value("book_id").toInt();
        summary.review.comment = query.value("comment").toString();
        summary.review.rating = query.value("rating").toInt();
        summary.review.date = query.value("date").toDateTime();
        summary.review.isApproved = query.value("is_approved").toBool();
        summary.bookTitle = query.value("book_title").toString();
        summary.username = query.value("username").toString();
        outReviews.push_back(summary);
    }
    return true;
}

bool DatabaseManager::approveReview(int reviewId, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("UPDATE reviews SET is_approved = 1 WHERE id = :id");
    query.bindValue(":id", reviewId);
    
    if (!query.exec()) {
        errorMsg = "Failed to approve review: " + query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::deleteReview(int reviewId, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("DELETE FROM reviews WHERE id = :id");
    query.bindValue(":id", reviewId);
    
    if (!query.exec()) {
        errorMsg = "Failed to delete review: " + query.lastError().text();
        return false;
    }
    
    return true;
}

bool DatabaseManager::addNotification(int userId, const QString &title, const QString &message, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("INSERT INTO notifications (user_id, title, message) VALUES (:uid, :title, :msg)");
    query.bindValue(":uid", userId);
    query.bindValue(":title", title);
    query.bindValue(":msg", message);
    if (!query.exec()) {
        errorMsg = "Failed to create notification: " + query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::fetchNotifications(int userId, QVector<Notification> &outNotifications, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("SELECT id, title, message, date, is_read FROM notifications WHERE user_id = :uid ORDER BY date DESC");
    query.bindValue(":uid", userId);
    if (!query.exec()) {
        errorMsg = "Database error while fetching notifications: " + query.lastError().text();
        return false;
    }
    outNotifications.clear();
    while (query.next()) {
        Notification n;
        n.id = query.value("id").toInt();
        n.userId = userId;
        n.title = query.value("title").toString();
        n.message = query.value("message").toString();
        n.date = query.value("date").toDateTime();
        n.isRead = query.value("is_read").toBool();
        outNotifications.push_back(n);
    }
    return true;
}

bool DatabaseManager::markNotificationRead(int notificationId, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("UPDATE notifications SET is_read = 1 WHERE id = :id");
    query.bindValue(":id", notificationId);
    if (!query.exec()) {
        errorMsg = "Failed to mark notification read: " + query.lastError().text();
        return false;
    }
    return true;
}
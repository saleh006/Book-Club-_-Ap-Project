#include <QJsonArray>
#include <QDateTime>
#include "databasemanager.h"
#include "clienthandler.h"

bool ClientHandler::handleReviewAndNotificationActions(const QString &action, const QJsonObject &requestObj, QJsonObject &responseObj)
{
    if (action == "submit_review") {
        responseObj["action"] = "submit_review_response";
        Review r;
        r.userId = requestObj["userId"].toInt();
        r.bookId = requestObj["bookId"].toInt();
        r.rating = requestObj["rating"].toInt();
        r.comment = requestObj["comment"].toString();

        if (r.rating < 1 || r.rating > 5) {
            responseObj["status"] = "error";
            responseObj["message"] = "Rating must be between 1 and 5.";
        } else {
            QString errorMsg;
            if (DatabaseManager::instance().addReview(r, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["message"] = "Review submitted and awaiting approval.";
                emit databaseUpdated("reviews");
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
    }
    else if (action == "get_book_reviews") {
        responseObj["action"] = "book_reviews_response";
        int bookId = requestObj["bookId"].toInt();
        QVector<Review> reviews;
        QString errorMsg;
        if (DatabaseManager::instance().fetchReviewsForBook(bookId, reviews, errorMsg)) {
            responseObj["status"] = "success";
            QJsonArray arr;
            for (const Review &r : reviews) {
                QJsonObject rObj;
                rObj["id"] = r.id;
                rObj["bookId"] = r.bookId;
                rObj["userId"] = r.userId;
                rObj["username"] = r.username;
                rObj["rating"] = r.rating;
                rObj["comment"] = r.comment;
                rObj["date"] = r.date.toString(Qt::ISODate);
                rObj["isApproved"] = r.isApproved;
                arr.append(rObj);
            }
            responseObj["data"] = arr;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "get_all_reviews_admin" || action == "get_pending_reviews") {
        bool pendingOnly = (action == "get_pending_reviews");
        responseObj["action"] = pendingOnly ? "pending_reviews_response" : "all_reviews_response";
        QVector<ReviewAdminSummary> reviews;
        QString errorMsg;
        if (DatabaseManager::instance().fetchReviewsForAdmin(pendingOnly, reviews, errorMsg)) {
            responseObj["status"] = "success";
            QJsonArray arr;
            for (const ReviewAdminSummary &s : reviews) {
                QJsonObject rObj;
                rObj["id"] = s.review.id;
                rObj["bookId"] = s.review.bookId;
                rObj["bookTitle"] = s.bookTitle;
                rObj["userId"] = s.review.userId;
                rObj["username"] = s.username;
                rObj["rating"] = s.review.rating;
                rObj["comment"] = s.review.comment;
                rObj["date"] = s.review.date.toString(Qt::ISODate);
                rObj["isApproved"] = s.review.isApproved;
                arr.append(rObj);
            }
            responseObj["data"] = arr;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "approve_review") {
        responseObj["action"] = "approve_review_response";
        int reviewId = requestObj["reviewId"].toInt();
        QString errorMsg;

        if (DatabaseManager::instance().approveReview(reviewId, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Review approved.";
            emit databaseUpdated("reviews");
        }
        else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "review_delete") {
        responseObj["action"] = "review_delete_response";
        int reviewId = requestObj["reviewId"].toInt();
        QString errorMsg;

        if (DatabaseManager::instance().deleteReview(reviewId, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Review removed.";
            emit databaseUpdated("reviews");
        }
        else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "notification_add") {
        int userId = requestObj["userId"].toInt();
        QString title = requestObj["title"].toString();
        QString message = requestObj["message"].toString();
        QString errorMsg;
        if (DatabaseManager::instance().addNotification(userId, title, message, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Notification sent successfully.";
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "notifications_fetch") {
        int userId = requestObj["userId"].toInt();
        QVector<Notification> notifications;
        QString errorMsg;
        if (DatabaseManager::instance().fetchNotifications(userId, notifications, errorMsg)) {
            responseObj["status"] = "success";
            QJsonArray notifArray;
            for (const Notification &n : notifications) {
                QJsonObject notifObj;
                notifObj["id"] = n.id;
                notifObj["title"] = n.title;
                notifObj["message"] = n.message;
                notifObj["isRead"] = n.isRead;
                notifArray.append(notifObj);
            }
            responseObj["notifications"] = notifArray;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "notification_mark_read") {
        int notificationId = requestObj["notificationId"].toInt();
        QString errorMsg;
        if (DatabaseManager::instance().markNotificationRead(notificationId, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Notification marked as read.";
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

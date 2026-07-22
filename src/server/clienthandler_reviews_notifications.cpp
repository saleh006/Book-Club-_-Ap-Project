#include <QJsonArray>
#include <QDateTime>
#include "databasemanager.h"
#include "clienthandler.h"

bool ClientHandler::handleReviewAndNotificationActions(const QString &action, const QJsonObject &requestObj, QJsonObject &responseObj)
{
    if (action == "review_add") {
        Review r;
        r.userId = requestObj["userId"].toInt();
        r.bookId = requestObj["bookId"].toInt();
        r.rating = requestObj["rating"].toInt();
        r.comment = requestObj["comment"].toString();
        r.date = QDateTime::currentDateTime();

        if (r.rating < 1 || r.rating > 5) {
            responseObj["status"] = "error";
            responseObj["message"] = "Rating must be between 1 and 5.";
        } else {
            QString errorMsg;
            if (DatabaseManager::instance().addReview(r, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["message"] = "Review and rating submitted successfully.";
                DatabaseManager::instance().recalculateAverageRating(r.bookId, errorMsg);
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
    }
    else if (action == "review_fetch_for_book") {
        int bookId = requestObj["bookId"].toInt();
        QVector<Review> reviews;
        QString errorMsg;
        if (DatabaseManager::instance().fetchReviewsForBook(bookId, reviews, errorMsg)) {
            responseObj["status"] = "success";
            QJsonArray reviewArray;
            for (const Review &r : reviews) {
                QJsonObject reviewObj;
                reviewObj["id"] = r.id;
                reviewObj["userId"] = r.userId;
                reviewObj["rating"] = r.rating;
                reviewObj["comment"] = r.comment;
                reviewObj["date"] = r.date.toString(Qt::ISODate);
                reviewArray.append(reviewObj);
            }
            responseObj["reviews"] = reviewArray;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "get_all_reviews_admin") {
        responseObj["action"] = "all_reviews_response";
        QVector<ReviewAdminSummary> reviews;
        QString errorMsg;

        if (DatabaseManager::instance().fetchReviewsForAdmin(false, reviews, errorMsg)) {
            responseObj["status"] = "success";
            QJsonArray reviewArray;
            for (const ReviewAdminSummary &r : reviews) {
                QJsonObject reviewObj;
                reviewObj["id"] = r.review.id;
                reviewObj["bookId"] = r.review.bookId;
                reviewObj["bookTitle"] = r.bookTitle;
                reviewObj["username"] = r.username;
                reviewObj["rating"] = r.review.rating;
                reviewObj["comment"] = r.review.comment;
                reviewObj["date"] = r.review.date.toString("yyyy-MM-dd");
                reviewObj["isApproved"] = r.review.isApproved;
                reviewArray.append(reviewObj);
            }
            responseObj["data"] = reviewArray;
        }
        else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "get_pending_reviews") {
        responseObj["action"] = "pending_reviews_response";
        QVector<ReviewAdminSummary> reviews;
        QString errorMsg;

        if (DatabaseManager::instance().fetchReviewsForAdmin(true, reviews, errorMsg)) {
            responseObj["status"] = "success";
            QJsonArray reviewArray;
            for (const ReviewAdminSummary &r : reviews) {
                QJsonObject reviewObj;
                reviewObj["id"] = r.review.id;
                reviewObj["bookId"] = r.review.bookId;
                reviewObj["bookTitle"] = r.bookTitle;
                reviewObj["username"] = r.username;
                reviewObj["rating"] = r.review.rating;
                reviewObj["comment"] = r.review.comment;
                reviewObj["date"] = r.review.date.toString("yyyy-MM-dd");
                reviewArray.append(reviewObj);
            }
            responseObj["data"] = reviewArray;
        }
        else {
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

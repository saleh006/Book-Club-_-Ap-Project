
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QDateTime>
#include "databasemanager.h"
#include "servermanager.h"

ClientHandler::ClientHandler(qintptr socketDescriptor, QObject *parent)
    : QThread(parent), m_socketDescriptor(socketDescriptor), m_socket(nullptr)
{}

ClientHandler::~ClientHandler()
{
    if (this->isRunning()) {
        this->quit();
        this->wait();
    }
}

void ClientHandler::run()
{
    qDebug() << "New thread created for client. Thread ID:" << QThread::currentThreadId();

    m_socket = new QTcpSocket();

    if (!m_socket->setSocketDescriptor(m_socketDescriptor)) {
        qWarning() << "Error setting client socket descriptor!";
        delete m_socket;
        m_socket = nullptr;
        return;
    }
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead, Qt::DirectConnection);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected, Qt::DirectConnection);

    exec();
}

void ClientHandler::onReadyRead()
{
    m_buffer.append(m_socket->readAll());
    int newlineIndex;
    while ((newlineIndex = m_buffer.indexOf('\n')) != -1) {
        QByteArray rawData = m_buffer.left(newlineIndex).trimmed();
        m_buffer.remove(0, newlineIndex + 1);
        if (rawData.isEmpty()) continue;
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(rawData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            m_socket->write("{\"status\":\"error\",\"message\":\"Invalid JSON format\"}\n");
            m_socket->flush();
            continue;
        }
        QJsonObject requestObj = jsonDoc.object();
        QString action = requestObj["action"].toString();
        QJsonObject responseObj;
        QString clientIp = m_socket ? m_socket->peerAddress().toString() : "Unknown IP";
        QString reqLog = QString("<font color='#3498db'><b>[REQ]</b></font> "
                                 "User: <b>%1</b> | IP: %2 | Action: <font color='#f1c40f'><b>%3</b></font>")
                             .arg(m_username)
                             .arg(clientIp)
                             .arg(action);
        emit logProduced(reqLog);

        // ========
        // USERS
        // ========
        if (action == "register") {
            QString username = requestObj["username"].toString();
            QString password = requestObj["password"].toString();
            QString fullName = requestObj["fullName"].toString();
            QString email = requestObj["email"].toString();
            QString recoveryAnswer = requestObj["recoveryAnswer"].toString();
            QString role = requestObj["role"].toString();
            QString errorMsg;

            if (DatabaseManager::instance().registerUser(username, password, fullName, email, recoveryAnswer, errorMsg, role)) {
                responseObj["status"] = "success";
                responseObj["message"] = "Registration successful. You can now log in.";
                emit databaseUpdated("users");
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
        else if (action == "login") {
            QString username = requestObj["username"].toString();
            QString password = requestObj["password"].toString();
            QString errorMsg;
            User loggedInUser;

            if (DatabaseManager::instance().authenticateUser(username, password, errorMsg, &loggedInUser)) {
                responseObj["status"] = "success";
                responseObj["message"] = "Login successful.";
                responseObj["role"] = loggedInUser.role;
                responseObj["fullName"] = loggedInUser.fullName;
                responseObj["userId"] = loggedInUser.id;
                m_username = username;
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
        else if (action == "user_fetch") {
            QString username = requestObj["username"].toString();
            User u;
            QString errorMsg;
            if (DatabaseManager::instance().fetchUser(username, u, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["fullName"] = u.fullName;
                responseObj["email"] = u.email;
                responseObj["role"] = u.role;
                responseObj["type"] = "user_info";
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
        else if (action == "set_user_block_status") {
            QString username = requestObj["username"].toString();
            bool blockStatus = requestObj["block_status"].toBool();
            QString errorMsg;

            if (DatabaseManager::instance().setUserBlocked(username, blockStatus, errorMsg)) {
                QJsonObject response;
                response["action"] = "set_user_block_status_response";
                response["status"] = "success";
                response["username"] = username;
                response["block_status"] = blockStatus;
                sendToClient(response);

                emit databaseUpdated("users");
                QString logMsg = QString("[ADMIN] User '%1' block status updated to: %2")
                                     .arg(username).arg(blockStatus ? "Blocked" : "Active");
                emit logProduced(logMsg);
            }
            else {
                QJsonObject response;
                response["action"] = "set_user_block_status_response";
                response["status"] = "error";
                response["message"] = errorMsg;
                sendToClient(response);
            }
        }
        else if (action == "recover_password") {
            QString username = requestObj["username"].toString();
            QString recoveryAnswer = requestObj["recoveryAnswer"].toString();
            QString newPassword = requestObj["newPassword"].toString();
            QString errorMsg;

            if (DatabaseManager::instance().resetPasswordWithRecovery(username, recoveryAnswer, newPassword, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["message"] = "Password reset successfully. You can now log in.";
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
        else if (action == "get_users_list") {
            QVector<UserProfileSummary> profiles;
            QString errorMsg;

            if (DatabaseManager::instance().fetchAllUsersProfilesForAdmin(profiles, errorMsg)) {
                QJsonArray usersArray;
                for (const auto &profile : profiles) {
                    QJsonObject userObj;
                    userObj["username"] = profile.user.username;
                    userObj["fullName"] = profile.user.fullName;
                    userObj["role"] = profile.user.role;
                    userObj["isBlocked"] = profile.user.isBlocked;
                    userObj["registerDate"] = profile.user.registerDate.toString("yyyy-MM-dd");

                    usersArray.append(userObj);
                }
                QJsonObject response;
                response["action"] = "users_list_response";
                response["status"] = "success";
                response["data"] = usersArray;
                sendToClient(response);
            } else {
                QJsonObject response;
                response["action"] = "users_list_response";
                response["status"] = "error";
                response["message"] = errorMsg;
                sendToClient(response);
            }
        }
        else if (action == "get_user_details") {
            QString targetUser = requestObj["username"].toString();
            QString errorMsg;
            UserProfileSummary profile;

            if (DatabaseManager::instance().fetchUserProfileForAdmin(targetUser, profile, errorMsg)) {
                QJsonObject data;
                data["username"] = profile.user.username;
                data["fullName"] = profile.user.fullName;
                data["email"] = profile.user.email;
                data["role"] = profile.user.role;
                data["isBlocked"] = profile.user.isBlocked;
                data["registerDate"] = profile.user.registerDate.toString("yyyy-MM-dd");
                data["ownedBooksCount"] = profile.ownedBooks.size();
                data["wishlistCount"] = profile.wishlist.size();
                data["totalPurchases"] = profile.purchaseHistory.size();

                data["cartItemsCount"] = profile.cartItems.size();
                data["cartTotal"] = profile.cartTotal;

                QJsonArray ownedArr;
                for (const Book &b : profile.ownedBooks) {
                    QJsonObject o;
                    o["title"] = b.title;
                    o["author"] = b.author;
                    ownedArr.append(o);
                }
                data["ownedBooks"] = ownedArr;

                QJsonArray wishlistArr;
                for (const Book &b : profile.wishlist) {
                    QJsonObject o;
                    o["title"] = b.title;
                    o["author"] = b.author;
                    wishlistArr.append(o);
                }
                data["wishlist"] = wishlistArr;

                QJsonArray purchasesArr;
                double totalSpent = 0.0;
                for (const Purchase &p : profile.purchaseHistory) {
                    QJsonObject o;
                    o["date"] = p.purchaseDate.toString("yyyy-MM-dd");
                    o["total"] = p.totalPrice;
                    o["itemCount"] = p.bookIds.size();
                    purchasesArr.append(o);
                    totalSpent += p.totalPrice;
                }
                data["purchaseHistory"] = purchasesArr;
                data["totalSpent"] = totalSpent;

                QJsonObject response;
                response["action"] = "user_details_response";
                response["status"] = "success";
                response["data"] = data;
                sendToClient(response);
            } else {
                QJsonObject response;
                response["action"] = "user_details_response";
                response["status"] = "error";
                response["message"] = errorMsg;
                sendToClient(response);
            }
        }
        else if (action == "admin_subscribe") {
            ServerManager *server = qobject_cast<ServerManager*>(parent());
            if (server) {
                connect(server, &ServerManager::broadcastToAdmins, this, &ClientHandler::sendToClient);
            }
        }
        else if (action == "user_update_profile") {
            int userId          = requestObj["userId"].toInt();
            QString newUsername = requestObj["newUsername"].toString();
            QString fullName    = requestObj["fullName"].toString();
            QString email       = requestObj["email"].toString();
            QString errorMsg;
            responseObj["type"] = "profile_update_result";
            if (DatabaseManager::instance().updateUserProfile(userId, newUsername, fullName, email, errorMsg)) {
                responseObj["success"]  = true;
                responseObj["username"] = newUsername;
                responseObj["fullName"] = fullName;
                responseObj["email"]    = email;
                m_username = newUsername;
                emit databaseUpdated("users");
            } else {
                responseObj["success"] = false;
                responseObj["message"] = errorMsg;
            }
        }
        else if (action == "user_change_password") {
            int userId          = requestObj["userId"].toInt();
            QString oldPassword = requestObj["oldPassword"].toString();
            QString newPassword = requestObj["newPassword"].toString();
            QString errorMsg;
            responseObj["type"] = "password_change_result";
            if (DatabaseManager::instance().changePassword(userId, oldPassword, newPassword, errorMsg)) {
                responseObj["success"] = true;
            } else {
                responseObj["success"] = false;
                responseObj["message"] = errorMsg;
            }
        }
        /*else if (action == "delete_account") {
            QString username = requestObj["username"].toString();
            QString errorMsg;
            if (DatabaseManager::instance().deleteUser(username, errorMsg)) {
                QJsonObject response;
                response["action"] = "delete_account_response";
                response["status"] = "success";
                sendToClient(response);

                emit databaseUpdated("users");
                emit databaseUpdated("publishers");
                QString logMsg = QString("[ADMIN] Account '%1' was permanently deleted.").arg(username);
                emit logProduced(logMsg);
            }
            else {
                QJsonObject response;
                response["action"] = "delete_account_response";
                response["status"] = "error";
                response["message"] = errorMsg;
                sendToClient(response);
            }
        }*/

        // ========
        // BOOKS
        // ========
        else if (action == "books_add") {
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
        else if (action == "books_delete") {
            responseObj["action"] = "delete_book_response";
            int bookId = requestObj["bookId"].toInt();
            QString errorMsg;
            if (DatabaseManager::instance().deleteBook(bookId, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["message"] = "Book deleted successfully.";
                emit logProduced(QString("[SYS] Book ID %1 was deleted.").arg(bookId));
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
                data["isActive"] = b.isActive;
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
                    bookObj["isActive"] = book.isActive;
                    bookObj["price"] = book.price;

                    booksArray.append(bookObj);
                }

                QJsonObject response;
                response["status"] = "success";
                response["data"] = booksArray;
                sendToClient(response);
            } else {
                QJsonObject response;
                response["status"] = "error";
                response["message"] = errorMsg;
                sendToClient(response);
            }
        }
        /*else if (action == "set_book_active_status") {
            int bookId = requestObj["bookId"].toInt();
            bool activeStatus = requestObj["active_status"].toBool();
            QString errorMsg;

            if (DatabaseManager::instance().updateBookActiveStatus(bookId, activeStatus, errorMsg)) {
                QJsonObject response;
                response["action"] = "set_book_active_status_response";
                response["status"] = "success";
                response["bookId"] = bookId;
                response["active_status"] = activeStatus;
                sendToClient(response);

                emit databaseUpdated("book");
                QString logMsg = QString("[ADMIN] Book ID %1 status updated to: %2")
                                     .arg(bookId).arg(activeStatus ? "Approved/Active" : "Rejected/Inactive");
                emit logProduced(logMsg);
            }
            else {
                QJsonObject response;
                response["action"] = "set_book_active_status_response";
                response["status"] = "error";
                response["message"] = errorMsg;
                sendToClient(response);
            }
        }*/
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

        // ============
        // DISCOUNTS
        // ============
        else if (action == "discount_add") {
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

        // ==========
        // REVIEWS
        // ==========
        else if (action == "review_add") {
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

        // ================
        // NOTIFICATIONS
        // ================
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

        // =====================
        // SHELVES & PROGRESS
        // =====================
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

        // ===========
        // WISHLIST)
        // ===========
        else if (action == "wishlist_add") {
            int userId = requestObj["userId"].toInt();
            int bookId = requestObj["bookId"].toInt();
            QString errorMsg;
            if (DatabaseManager::instance().addToWishlist(userId, bookId, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["message"] = "Added to wishlist.";
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
        else if (action == "wishlist_remove") {
            int userId = requestObj["userId"].toInt();
            int bookId = requestObj["bookId"].toInt();
            QString errorMsg;
            if (DatabaseManager::instance().removeFromWishlist(userId, bookId, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["message"] = "Removed from wishlist.";
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
        else if (action == "wishlist_fetch") {
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

        // ================
        // SHOPPING CART
        // ================
        else if (action == "add_to_cart") {
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
                    cartArray.append(itemObj);
                }
                responseObj["items"] = cartArray;
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }

        // ============
        // PURCHASES
        // ============
        else if (action == "checkout") {
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

        // ============
        // PUBLISHER
        // ============
        else if (action == "publisher_fetch_books") {
            int publisherId = requestObj["publisherId"].toInt();
            bool activeOnly = requestObj["activeOnly"].toBool(false);
            QVector<Book> books;
            QString errorMsg;
            if (DatabaseManager::instance().fetchPublishedBooks(publisherId, books, errorMsg, activeOnly)) {
                responseObj["status"] = "success";
                QJsonArray bookArray;
                for (const Book &b : books) {
                    QJsonObject bObj;
                    bObj["id"] = b.id;
                    bObj["title"] = b.title;
                    bObj["totalSales"] = b.totalSales;
                    bookArray.append(bObj);
                }
                responseObj["books"] = bookArray;
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
        else if (action == "publisher_fetch_book_income") {
            int publisherId = requestObj["publisherId"].toInt();
            int bookId = requestObj["bookId"].toInt();
            double income = 0.0;
            QString errorMsg;
            if (DatabaseManager::instance().fetchPublisherIncomeForBook(publisherId, bookId, income, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["income"] = income;
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
        // else if (action == "publisher_get_stats") {
        //     int publisherId = requestObj["publisherId"].toInt();
        //     int bookCount = 0;
        //     int totalSales = 0;
        //     double averageRating = 0.0;
        //     double totalIncome = 0.0;
        //     QString errorMsg;

        //     bool statsSuccess = DatabaseManager::instance().fetchPublisherStats(publisherId, bookCount, totalSales, averageRating, errorMsg);
        //     bool incomeSuccess = DatabaseManager::instance().fetchPublisherIncome(publisherId, totalIncome, errorMsg);
        //     if (statsSuccess && incomeSuccess) {
        //         responseObj["status"] = "success";
        //         responseObj["bookCount"] = bookCount;
        //         responseObj["totalSales"] = totalSales;
        //         responseObj["averageRating"] = averageRating;
        //         responseObj["totalIncome"] = totalIncome;
        //     } else {
        //         responseObj["status"] = "error";
        //         responseObj["message"] = errorMsg.isEmpty() ? "Error fetching publisher stats." : errorMsg;
        //     }
        // }
        else if (action == "book_set_ownership") {
            int bookId = requestObj["bookId"].toInt();
            int publisherId = requestObj["publisherId"].toInt();
            QString errorMsg;
            if (DatabaseManager::instance().setBookOwnership(bookId, publisherId, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["message"] = "Book ownership changed successfully.";
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
        else if (action == "publisher_get_books") {
            int publisherId = requestObj["publisherId"].toInt();
            QVector<Book> books;
            QString errorMsg;
            DatabaseManager::instance().fetchPublishedBooks(publisherId, books, errorMsg, false);

            QJsonArray booksArray;
            for (const Book &b : std::as_const(books)) {
                QJsonObject bo;
                bo["id"] = b.id;
                bo["title"] = b.title;
                bo["author"] = b.author;
                bo["genre"] = b.genre;
                bo["description"] = b.description;
                bo["price"] = b.price;
                bo["coverImagePath"] = b.coverImagePath;
                bo["pdfPath"] = b.pdfPath;
                bo["status"] = b.status;
                bo["averageRating"] = b.averageRating;
                bo["totalSales"] = b.totalSales;
                booksArray.append(bo);
            }
            responseObj["type"] = "publisher_books_list";
            responseObj["books"] = booksArray;
        }

        else if (action == "publisher_get_stats") {
            int publisherId = requestObj["publisherId"].toInt();
            int bookCount = 0, totalSales = 0;
            double avgRating = 0.0, totalIncome = 0.0;
            QString errorMsg;

            DatabaseManager::instance().fetchPublisherStats(publisherId, bookCount, totalSales, avgRating, errorMsg);
            DatabaseManager::instance().fetchPublisherIncome(publisherId, totalIncome, errorMsg);

            responseObj["type"] = "publisher_stats";
            responseObj["bookCount"] = bookCount;
            responseObj["totalSales"] = totalSales;
            responseObj["averageRating"] = avgRating;
            responseObj["totalIncome"] = totalIncome;
        }

        else if (action == "publisher_add_book") {
            Book book;
            book.publisherId = requestObj["publisherId"].toInt();
            book.title = requestObj["title"].toString();
            book.author = requestObj["author"].toString();
            book.genre = requestObj["genre"].toString();
            book.description = requestObj["description"].toString();
            book.price = requestObj["price"].toDouble();
            book.coverImagePath = requestObj["coverImagePath"].toString();
            book.pdfPath = requestObj["pdfPath"].toString();

            int newBookId = -1;
            QString errorMsg;
            if (DatabaseManager::instance().addBook(book, newBookId, errorMsg)) {
                responseObj["type"] = "action_result";
                responseObj["success"] = true;
                responseObj["newBookId"] = newBookId;
                responseObj["message"] = "Book added.";
            } else {
                responseObj["type"] = "action_result";
                responseObj["success"] = false;
                responseObj["message"] = errorMsg;
            }
        }

        else if (action == "publisher_update_book") {
            Book book;
            book.id = requestObj["id"].toInt();
            book.publisherId = requestObj["publisherId"].toInt();
            book.title = requestObj["title"].toString();
            book.author = requestObj["author"].toString();
            book.genre = requestObj["genre"].toString();
            book.description = requestObj["description"].toString();
            book.price = requestObj["price"].toDouble();
            book.coverImagePath = requestObj["coverImagePath"].toString();
            book.pdfPath = requestObj["pdfPath"].toString();
            // remove the "book.isActive = true;" line entirely,
            // OR if publishers should be able to resubmit for review:
            book.status = requestObj.contains("status") ? requestObj["status"].toInt() : 1;

            QString errorMsg;
            bool ok = DatabaseManager::instance().updateBook(book, errorMsg);
            responseObj["type"] = "action_result";
            responseObj["success"] = ok;
            responseObj["message"] = ok ? "Book updated." : errorMsg;
        }

        else if (action == "publisher_delete_book") {
            int bookId = requestObj["bookId"].toInt();
            QString errorMsg;
            bool ok = DatabaseManager::instance().deleteBook(bookId, errorMsg);
            responseObj["type"] = "action_result";
            responseObj["success"] = ok;
            responseObj["message"] = ok ? "Book removed." : errorMsg;
        }
        else if (action == "admin_set_book_status") {
            int bookId = requestObj["bookId"].toInt();
            int status = requestObj["status"].toInt(); // 1, 0, or -1
            QString errorMsg;
            if (DatabaseManager::instance().setBookStatus(bookId, status, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["message"] = "Book status updated.";
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
        else if (action == "publisher_set_book_status") {
            int bookId = requestObj["bookId"].toInt();
            int publisherId = requestObj["publisherId"].toInt();
            int status = requestObj["status"].toInt();

            if (status != 0 && status != 1) {
                responseObj["type"] = "action_result";
                responseObj["success"] = false;
                responseObj["message"] = "Publishers can only set active/inactive.";
            } else {
                // Verify the book actually belongs to this publisher before touching it
                Book b;
                QString fetchErr;
                if (!DatabaseManager::instance().fetchBook(bookId, b, fetchErr) || b.publisherId != publisherId) {
                    responseObj["type"] = "action_result";
                    responseObj["success"] = false;
                    responseObj["message"] = "Book not found or not owned by you.";
                } else {
                    QString errorMsg;
                    bool ok = DatabaseManager::instance().setBookStatus(bookId, status, errorMsg);
                    responseObj["type"] = "action_result";
                    responseObj["success"] = ok;
                    responseObj["message"] = ok ? "Book status updated." : errorMsg;
                }
            }
        }
        else if (action == "publisher_add_discount") {
            int publisherId = requestObj["publisherId"].toInt();
            int bookId = requestObj["bookId"].toInt();

            // Verify the book actually belongs to this publisher before allowing a discount on it
            Book b;
            QString fetchErr;
            if (!DatabaseManager::instance().fetchBook(bookId, b, fetchErr) || b.publisherId != publisherId) {
                responseObj["status"] = "error";
                responseObj["message"] = "Book not found or not owned by you.";
            } else {
                Discount d;
                d.bookId = bookId;
                d.type = requestObj["type"].toString();
                d.value = requestObj["value"].toDouble();
                d.startDate = QDateTime::fromString(requestObj["startDate"].toString(), Qt::ISODate);
                d.endDate = QDateTime::fromString(requestObj["endDate"].toString(), Qt::ISODate);

                QString errorMsg;
                if (DatabaseManager::instance().addDiscount(d, errorMsg)) {
                    responseObj["status"] = "success";
                    responseObj["message"] = "Offer set successfully.";
                } else {
                    responseObj["status"] = "error";
                    responseObj["message"] = errorMsg;
                }
            }
        }
        else if (action == "upload_file") {
            QString fileType = requestObj["fileType"].toString(); // "cover" or "pdf"
            QString fileName = requestObj["fileName"].toString();
            QByteArray fileBytes = QByteArray::fromBase64(requestObj["fileData"].toString().toUtf8());

            QString subfolder = (fileType == "cover") ? "covers" : "pdfs";
            QString storageDir = QCoreApplication::applicationDirPath() + "/uploads/" + subfolder;
            QDir().mkpath(storageDir); // creates the folder if it doesn't exist yet
            QString uniqueName = QString::number(QDateTime::currentMSecsSinceEpoch()) + "_" + fileName;
            QString fullPath = storageDir + "/" + uniqueName;

            QFile outFile(fullPath);
            if (outFile.open(QIODevice::WriteOnly)) {
                outFile.write(fileBytes);
                outFile.close();
                responseObj["type"] = "upload_result";
                responseObj["success"] = true;
                responseObj["serverPath"] = fullPath;
            } else {
                responseObj["type"] = "upload_result";
                responseObj["success"] = false;
                responseObj["message"] = "Failed to save file on server.";
            }
        }
        else if (action == "publisher_get_sales_trend") {
            int publisherId = requestObj["publisherId"].toInt();
            QString granularity = requestObj["granularity"].toString("monthly");
            QVector<QPair<QString, int>> points;
            QString errorMsg;
            DatabaseManager::instance().fetchPublisherSalesTrend(publisherId, granularity, points, errorMsg);
            QJsonArray arr;
            for (const auto &p : std::as_const(points)) {
                QJsonObject o;
                o["period"] = p.first;
                o["sales"]  = p.second;
                arr.append(o);
            }
            responseObj["type"] = "publisher_sales_trend";
            responseObj["points"] = arr;
        else if (action == "get_publishers_list") {
            QVector<PublisherProfileSummary> profiles;
            QString errorMsg;

            if (DatabaseManager::instance().fetchAllPublisherProfilesForAdmin(profiles, errorMsg)) {
                QJsonArray pubsArray;
                for (const auto &profile : profiles) {
                    QJsonObject pubObj;
                    pubObj["username"] = profile.user.username;
                    pubObj["fullName"] = profile.user.fullName;
                    pubObj["isBlocked"] = profile.user.isBlocked;
                    pubObj["registerDate"] = profile.user.registerDate.toString("yyyy-MM-dd");
                    pubObj["publishedBooksCount"] = profile.publishedBooks.size();

                    pubsArray.append(pubObj);
                }
                QJsonObject response;
                response["action"] = "publishers_list_response";
                response["status"] = "success";
                response["data"] = pubsArray;
                sendToClient(response);
            } else {
                QJsonObject response;
                response["action"] = "publishers_list_response";
                response["status"] = "error";
                response["message"] = errorMsg;
                sendToClient(response);
            }
        }
        else if (action == "get_publisher_details") {
            responseObj["action"] = "publisher_details_response";
            QString username = requestObj["username"].toString();
            QString errorMsg;
            PublisherProfileSummary profile;

            if (DatabaseManager::instance().fetchPublisherProfileForAdmin(username, profile, errorMsg)) {
                responseObj["status"] = "success";
                QJsonObject data;

                data["username"] = profile.user.username;
                data["fullName"] = profile.user.fullName;
                data["email"] = profile.user.email;
                data["registerDate"] = profile.user.registerDate.toString("yyyy-MM-dd");
                data["isBlocked"] = profile.user.isBlocked;
                data["publishedBooksCount"] = profile.publishedBooks.size();
                data["totalSales"] = profile.totalSales;
                data["averageRating"] = profile.averageRating;
                data["totalIncome"] = profile.totalIncome;

                QJsonArray booksArr;
                for (const Book &b : profile.publishedBooks) {
                    QJsonObject o;
                    o["title"] = b.title;
                    o["genre"] = b.genre;
                    o["price"] = b.price;
                    o["totalSales"] = b.totalSales;
                    o["averageRating"] = b.averageRating;
                    o["isActive"] = b.isActive;
                    booksArr.append(o);
                }
                data["publishedBooks"] = booksArr;

                responseObj["data"] = data;
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg.isEmpty() ? "Publisher not found." : errorMsg;
            }
        }
        else {
            responseObj["status"] = "error";
            responseObj["message"] = "Invalid action.";
        }

        QJsonDocument responseDoc(responseObj);
        m_socket->write(responseDoc.toJson(QJsonDocument::Compact) + "\n");
        m_socket->flush();

        QString status = responseObj["status"].toString();
        QString msg = responseObj["message"].toString();
        QString resLog;
        if (status == "success") {
            resLog = QString("<font color='#2ecc71'><b>[RES]</b></font> Action: <b>%1</b> | Status: SUCCESS")
            .arg(action);
            if (!msg.isEmpty()) resLog += QString(" (%1)").arg(msg);
        }
        else {
            resLog = QString("<font color='#e74c3c'><b>[ERR]</b></font> Action: <b>%1</b> | Status: ERROR | Reason: <i>%2</i>")
            .arg(action)
                .arg(msg.isEmpty() ? "Unknown Error" : msg);
        }
        QString finalLog = resLog + "<hr style='border: 0; border-top: 1px solid #3a3a4c; margin: 4px 0;'>";
        emit logProduced(finalLog);
    }
}

void ClientHandler::onDisconnected(){
    qDebug() << "Client disconnected. Cleaning up memory...";
    emit clientDisconnectedSignal(m_socketDescriptor,m_username);
    if(m_socket){
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    quit();
}

void ClientHandler::sendToClient(const QJsonObject &msg)
{
    if (m_socket && m_socket->isOpen()) {
        m_socket->write(QJsonDocument(msg).toJson(QJsonDocument::Compact) + "\n");
        m_socket->flush();
    }
}

ServerManager::ServerManager(QObject *parent)
    : QTcpServer(parent)
{
    connect(this, &ServerManager::serverLogEvent, this, [this](const QString &msg) {
        QJsonObject obj;
        obj["type"] = "log";
        obj["message"] = msg;
        emit broadcastToAdmins(obj);
    });

    connect(this, &ServerManager::clientCountChanged, this, [this](int count) {
        QJsonObject obj;
        obj["type"] = "client_count";
        obj["count"] = count;
        emit broadcastToAdmins(obj);
    });

    connect(this, &ServerManager::databaseUpdated, this, [this](const QString &type) {
        QJsonObject obj;
        obj["type"] = "table_refresh_required";
        obj["target_table"] = type;
        emit broadcastToAdmins(obj);
    });
}

bool ServerManager::startServer(int port)
{
    QSqlQuery walQuery;
    if (!walQuery.exec("PRAGMA journal_mode=WAL;")) {
        qWarning() << "Error enabling database WAL mode:" << walQuery.lastError().text();
    } else {
        qDebug() << "Database optimized: WAL mode enabled for thread concurrency management.";
    }

    if (!this->listen(QHostAddress::Any, port)) {
        qWarning() << "Server failed to start on port" << port << ": " << this->errorString();
        return false;
    }

    qDebug() << "BookClub server successfully started on port" << port << "...";
    return true;
}

void ServerManager::stopServer()
{
    if (this->isListening()) {
        this->close();
        qDebug() << "Server stopped listening on port" << this->serverPort();
    }
}

void ServerManager::incomingConnection(qintptr socketDescriptor)
{
    ClientHandler *handler = new ClientHandler(socketDescriptor,this);
    QString connectLog = QString("<font color='#7f8c8d'><b>[SYS]</b></font> New client connected. User: <b>Anonymous</b> | Descriptor: %1")
                             .arg(socketDescriptor);
    emit serverLogEvent(connectLog);

    m_activeClients++;
    emit clientCountChanged(m_activeClients);

    connect(handler,&ClientHandler::logProduced,this,&ServerManager::serverLogEvent);
    connect(handler, &ClientHandler::databaseUpdated,this,&ServerManager::databaseUpdated);
    connect(handler, &ClientHandler::clientDisconnectedSignal,this,[this](qintptr desc,const QString &username){
        if(m_activeClients > 0) m_activeClients--;
        emit clientCountChanged(m_activeClients);
        emit serverLogEvent(QString("<font color='#7f8c8d'><b>[SYS]</b></font> User <b>%1</b> disconnected.").arg(username));
    });

    connect(handler, &ClientHandler::finished, handler, &ClientHandler::deleteLater);
    handler->start();
}
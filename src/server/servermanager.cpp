
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
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
        emit logProduced(QString("[Request] Client (Thread %1) requested action: %2").arg(size_t(QThread::currentThreadId())).arg(action));

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
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
        else if (action == "admin_set_blocked") {
            QString username = requestObj["username"].toString();
            bool blocked = requestObj["blocked"].toBool();
            QString errorMsg;
            if (DatabaseManager::instance().setUserBlocked(username, blocked, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["message"] = blocked ? "User successfully blocked." : "User successfully unblocked.";
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
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
                } else {
                    responseObj["status"] = "error";
                    responseObj["message"] = errorMsg;
                }
            }
        }
        else if (action == "book_update") {
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
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
        else if (action == "books_delete") {
            int bookId = requestObj["bookId"].toInt();
            QString errorMsg;
            if (DatabaseManager::instance().deleteBook(bookId, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["message"] = "Book deleted successfully.";
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
        else if (action == "book_fetch") {
            int bookId = requestObj["bookId"].toInt();
            Book b;
            QString errorMsg;
            if (DatabaseManager::instance().fetchBook(bookId, b, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["title"] = b.title;
                responseObj["author"] = b.author;
                responseObj["genre"] = b.genre;
                responseObj["description"] = b.description;
                responseObj["price"] = b.price;
                responseObj["coverImagePath"] = b.coverImagePath;
                responseObj["pdfPath"] = b.pdfPath;
                responseObj["averageRating"] = b.averageRating;
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
        else if (action == "books_fetch_all") {
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
        else if (action == "publisher_get_stats") {
            int publisherId = requestObj["publisherId"].toInt();
            int bookCount = 0;
            int totalSales = 0;
            double averageRating = 0.0;
            double totalIncome = 0.0;
            QString errorMsg;

            bool statsSuccess = DatabaseManager::instance().fetchPublisherStats(publisherId, bookCount, totalSales, averageRating, errorMsg);
            bool incomeSuccess = DatabaseManager::instance().fetchPublisherIncome(publisherId, totalIncome, errorMsg);
            if (statsSuccess && incomeSuccess) {
                responseObj["status"] = "success";
                responseObj["bookCount"] = bookCount;
                responseObj["totalSales"] = totalSales;
                responseObj["averageRating"] = averageRating;
                responseObj["totalIncome"] = totalIncome;
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg.isEmpty() ? "Error fetching publisher stats." : errorMsg;
            }
        }
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
        else {
            responseObj["status"] = "error";
            responseObj["message"] = "Invalid action.";
        }

        QJsonDocument responseDoc(responseObj);
        m_socket->write(responseDoc.toJson(QJsonDocument::Compact) + "\n");
        m_socket->flush();
        emit logProduced(QString("[Response] Sent status back: %1").arg(responseObj["status"].toString()));
    }
}

void ClientHandler::onDisconnected(){
    qDebug() << "Client disconnected. Cleaning up memory...";
    emit clientDisconnectedSignal(m_socketDescriptor);
    if(m_socket){
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    quit();
}

ServerManager::ServerManager(QObject *parent)
    : QTcpServer(parent)
{}

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

void ServerManager::incomingConnection(qintptr socketDescriptor)
{
    ClientHandler *handler = new ClientHandler(socketDescriptor);
    m_activeClients++;
    emit clientCountChanged(m_activeClients);
    connect(handler, &ClientHandler::clientDisconnectedSignal,this,[this](qintptr desc){
        if(m_activeClients > 0) m_activeClients--;
        emit clientCountChanged(m_activeClients);
        emit serverLogEvent(QString("Client with descriptor %1 disconnected.").arg(desc));
    });
    connect(handler, &ClientHandler::finished, handler, &ClientHandler::deleteLater);
    handler->start();
    emit serverLogEvent(QString("New client connected. Descriptor: %1").arg(socketDescriptor));
}
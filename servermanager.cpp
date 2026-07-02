
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include "databasemanager.h"
#include "servermanager.h"

ClientHandler::ClientHandler(qintptr socketDescriptor, QObject *parent)
    : QThread(parent), m_socketDescriptor(socketDescriptor), m_socket(nullptr)
{
    this->start();
}

ClientHandler::~ClientHandler()
{
    if (this->isRunning()) {
        this->quit();
        this->wait();
    }
}

void ClientHandler::run()
{
    qDebug() << "ترد جدید برای کلاینت ساخته شد. آی‌دی ترد:" << QThread::currentThreadId();

    m_socket = new QTcpSocket();

    if (!m_socket->setSocketDescriptor(m_socketDescriptor)) {
        qWarning() << "خطا در تنظیم شناسه سوکت کلاینت!";
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
    QByteArray rawData = m_socket->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(rawData);

    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        m_socket->write("Error: Invalid JSON format\n");
        m_socket->flush();
        return;
    }

    QJsonObject requestObj = jsonDoc.object();
    QString action = requestObj["action"].toString();
    QJsonObject responseObj;

    if (action == "login") {
        QString username = requestObj["username"].toString();
        QString password = requestObj["password"].toString();

        QString errorMsg;
        User loggedInUser;
        bool isSuccess = DatabaseManager::instance().authenticateUser(username, password, errorMsg, &loggedInUser);

        if (isSuccess) {
            responseObj["status"] = "success";
            responseObj["message"] = "ورود موفقیت‌آمیز بود.";
            responseObj["role"] = loggedInUser.role;
            responseObj["fullName"] = loggedInUser.fullName;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "register") {
        QString username = requestObj["username"].toString();
        QString password = requestObj["password"].toString();
        QString fullName = requestObj["fullName"].toString();
        QString email = requestObj["email"].toString();
        QString recoveryAnswer = requestObj["recoveryAnswer"].toString();
        QString role = requestObj["role"].toString();
        QString errorMsg;

        bool isSuccess = DatabaseManager::instance().registerUser(
            username, password, fullName, email, recoveryAnswer, errorMsg, role
            );
        if (isSuccess) {
            responseObj["status"] = "success";
            responseObj["message"] = "ثبت‌نام با موفقیت انجام شد. اکنون می‌توانید وارد شوید.";
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
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
            responseObj["message"] = "قیمت کتاب نمی‌تواند منفی باشد.";
        } else {
            int newBookId = -1;
            QString errorMsg;
            if (DatabaseManager::instance().addBook(b, newBookId, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["message"] = "کتاب با موفقیت ثبت شد.";
                responseObj["bookId"] = newBookId;
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
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
    else if (action == "add_to_cart") {
        int userId = requestObj["userId"].toInt();
        int bookId = requestObj["bookId"].toInt();
        int quantity = requestObj["quantity"].toInt(1);

        if (quantity < 1) {
            responseObj["status"] = "error";
            responseObj["message"] = "تعداد باید حداقل ۱ باشد.";
        } else {
            QString errorMsg;
            if (DatabaseManager::instance().addToCart(userId, bookId, quantity, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["message"] = "کتاب با موفقیت به سبد خرید اضافه شد.";
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = errorMsg;
            }
        }
    }
    else if (action == "checkout") {
        int userId = requestObj["userId"].toInt();
        QString errorMsg;
        int purchaseId = -1;
        if (DatabaseManager::instance().checkoutCart(userId, errorMsg, purchaseId)) {
            responseObj["status"] = "success";
            responseObj["message"] = "خرید شما با موفقیت انجام شد و سبد خرید خالی گردید.";
            responseObj["purchaseId"] = purchaseId;
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
            responseObj["message"] = blocked ? "کاربر با موفقیت بلاک شد." : "کاربر با موفقیت آنبلاک شد.";
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

        bool statsSuccess = DatabaseManager::instance().fetchPublisherStats(
            publisherId, bookCount, totalSales, averageRating, errorMsg
            );
        bool incomeSuccess = DatabaseManager::instance().fetchPublisherIncome(
            publisherId, totalIncome, errorMsg
            );
        if (statsSuccess && incomeSuccess) {
            responseObj["status"] = "success";
            responseObj["bookCount"] = bookCount;
            responseObj["totalSales"] = totalSales;
            responseObj["averageRating"] = averageRating;
            responseObj["totalIncome"] = totalIncome;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg.isEmpty() ? "خطا در دریافت آمار ناشر" : errorMsg;
        }
    }
    else if (action == "review_add") {
        Review r;
        r.userId = requestObj["userId"].toInt();
        r.bookId = requestObj["bookId"].toInt();
        r.rating = requestObj["rating"].toInt();
        r.comment = requestObj["comment"].toString();
        r.date = QDateTime::currentDateTime();

        if (r.rating < 1 || r.rating > 5) {
            responseObj["status"] = "error";
            responseObj["message"] = "امتیاز باید بین ۱ تا ۵ باشد.";
        } else {
            QString errorMsg;
            if (DatabaseManager::instance().addReview(r, errorMsg)) {
                responseObj["status"] = "success";
                responseObj["message"] = "نظر و امتیاز شما با موفقیت ثبت شد.";
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
    else {
        responseObj["status"] = "error";
        responseObj["message"] = "عملیات نامعتبر است.";
    }

    QJsonDocument responseDoc(responseObj);
    m_socket->write(responseDoc.toJson(QJsonDocument::Compact));
    m_socket->flush();
}

void ClientHandler::onDisconnected()
{
    qDebug() << "کلاینت ارتباطش را قطع کرد. پاکسازی حافظه...";
    m_socket->close();
    m_socket->deleteLater();
    this->quit();
    this->deleteLater();
}

ServerManager::ServerManager(QObject *parent)
    : QTcpServer(parent)
{}

bool ServerManager::startServer(int port)
{
    QSqlQuery walQuery;
    if (!walQuery.exec("PRAGMA journal_mode=WAL;")) {
        qWarning() << "خطا در فعالسازی حالت WAL دیتابیس:" << walQuery.lastError().text();
    } else {
        qDebug() << "دیتابیس دگرگون شد: حالت WAL برای مدیریت هم‌زمانی تردها فعال است.";
    }

    if (!this->listen(QHostAddress::Any, port)) {
        qWarning() << "سرور روی پورت" << port << "روشن نشد:" << this->errorString();
        return false;
    }

    qDebug() << "سرور باشگاه کتاب با موفقیت روی پورت" << port << "شروع به کار کرد...";
    return true;
}

void ServerManager::incomingConnection(qintptr socketDescriptor)
{
    new ClientHandler(socketDescriptor, this);
}
#include <QJsonArray>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include "databasemanager.h"
#include "clienthandler.h"

bool ClientHandler::handlePublisherActions(const QString &action, const QJsonObject &requestObj, QJsonObject &responseObj)
{
    if (action == "publisher_fetch_books") {
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

    // else if (action == "publisher_add_book") {
    //     Book book;
    //     book.publisherId = requestObj["publisherId"].toInt();
    //     book.title = requestObj["title"].toString();
    //     book.author = requestObj["author"].toString();
    //     book.genre = requestObj["genre"].toString();
    //     book.description = requestObj["description"].toString();
    //     book.price = requestObj["price"].toDouble();
    //     book.coverImagePath = requestObj["coverImagePath"].toString();
    //     book.pdfPath = requestObj["pdfPath"].toString();

    //     int newBookId = -1;
    //     QString errorMsg;
    //     if (DatabaseManager::instance().addBook(book, newBookId, errorMsg)) {
    //         responseObj["type"] = "action_result";
    //         responseObj["success"] = true;
    //         responseObj["newBookId"] = newBookId;
    //         responseObj["message"] = "Book added.";
    //         emit databaseUpdated("book");
    //     } else {
    //         responseObj["type"] = "action_result";
    //         responseObj["success"] = false;
    //         responseObj["message"] = errorMsg;
    //     }
    // }

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
            emit databaseUpdated("book");

            QJsonObject broadcastObj;
            broadcastObj["action"] = "notify_book_updated";
            broadcastObj["bookId"] = newBookId;
            broadcastObj["title"] = book.title;
            broadcastObj["author"] = book.author;
            broadcastObj["genre"] = book.genre;
            broadcastObj["price"] = book.price;
            broadcastObj["coverImagePath"] = book.coverImagePath;
            broadcastObj["status"] = 1;

            emit broadcastTargetedUpdate(broadcastObj);
        } else {
            responseObj["type"] = "action_result";
            responseObj["success"] = false;
            responseObj["message"] = errorMsg;
        }
    }

    // else if (action == "publisher_update_book") {
    //     Book book;
    //     book.id = requestObj["id"].toInt();
    //     book.publisherId = requestObj["publisherId"].toInt();
    //     book.title = requestObj["title"].toString();
    //     book.author = requestObj["author"].toString();
    //     book.genre = requestObj["genre"].toString();
    //     book.description = requestObj["description"].toString();
    //     book.price = requestObj["price"].toDouble();
    //     book.coverImagePath = requestObj["coverImagePath"].toString();
    //     book.pdfPath = requestObj["pdfPath"].toString();
    //     // remove the "book.isActive = true;" line entirely,
    //     // OR if publishers should be able to resubmit for review:
    //     book.status = requestObj.contains("status") ? requestObj["status"].toInt() : 1;

    //     QString errorMsg;
    //     bool ok = DatabaseManager::instance().updateBook(book, errorMsg);
    //     responseObj["type"] = "action_result";
    //     responseObj["success"] = ok;
    //     responseObj["message"] = ok ? "Book updated." : errorMsg;
    //     emit databaseUpdated("book");
    // }
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
        book.status = requestObj.contains("status") ? requestObj["status"].toInt() : 1;

        QString errorMsg;
        bool ok = DatabaseManager::instance().updateBook(book, errorMsg);
        responseObj["type"] = "action_result";
        responseObj["success"] = ok;
        responseObj["message"] = ok ? "Book updated." : errorMsg;
        emit databaseUpdated("book");

        if (ok) {
            QJsonObject broadcastObj;
            broadcastObj["action"] = "notify_book_updated";
            broadcastObj["bookId"] = book.id;
            broadcastObj["title"] = book.title;
            broadcastObj["author"] = book.author;
            broadcastObj["genre"] = book.genre;
            broadcastObj["price"] = book.price;
            broadcastObj["coverImagePath"] = book.coverImagePath;
            broadcastObj["status"] = book.status;

            emit broadcastTargetedUpdate(broadcastObj);
        }
    }

    // else if (action == "publisher_delete_book") {
    //     int bookId = requestObj["bookId"].toInt();
    //     QString errorMsg;
    //     bool ok = DatabaseManager::instance().deleteBook(bookId, errorMsg);
    //     responseObj["type"] = "action_result";
    //     responseObj["success"] = ok;
    //     responseObj["message"] = ok ? "Book removed." : errorMsg;
    //     emit databaseUpdated("book");
    // }
    else if (action == "publisher_delete_book") {
        int bookId = requestObj["bookId"].toInt();
        QString errorMsg;
        bool ok = DatabaseManager::instance().deleteBook(bookId, errorMsg);
        responseObj["type"] = "action_result";
        responseObj["success"] = ok;
        responseObj["message"] = ok ? "Book removed." : errorMsg;
        emit databaseUpdated("book");

        if (ok) {
            QJsonObject broadcastObj;
            broadcastObj["action"] = "notify_book_removed";
            broadcastObj["bookId"] = bookId;
            emit broadcastTargetedUpdate(broadcastObj);
        }
    }
    // else if (action == "admin_set_book_status") {
    //     responseObj["action"] = "admin_set_book_status_response";
    //     int bookId = requestObj["bookId"].toInt();
    //     int status = requestObj["status"].toInt(); // 1, 0, or -1
    //     QString errorMsg;

    //     if (DatabaseManager::instance().setBookStatus(bookId, status, errorMsg)) {
    //         responseObj["status"] = "success";
    //         responseObj["bookId"] = bookId;
    //         responseObj["book_status"] = status;

    //         QString actionLabel = status == -1 ? "deleted" : (status == 1 ? "approved" : "rejected");
    //         responseObj["message"] = QString("Book %1 successfully.").arg(actionLabel);

    //         emit databaseUpdated("book");
    //         emit logProduced(QString("[ADMIN] Book ID %1 was %2.").arg(bookId).arg(actionLabel));
    //     } else {
    //         responseObj["status"] = "error";
    //         responseObj["message"] = errorMsg;
    //     }
    // }

    else if (action == "admin_set_book_status") {
        responseObj["action"] = "admin_set_book_status_response";
        int bookId = requestObj["bookId"].toInt();
        int status = requestObj["status"].toInt(); // 1 (approved), 0 (rejected), -1 (deleted)
        QString errorMsg;

        if (DatabaseManager::instance().setBookStatus(bookId, status, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["bookId"] = bookId;
            responseObj["book_status"] = status;

            QString actionLabel = status == -1 ? "deleted" : (status == 1 ? "approved" : "rejected");
            responseObj["message"] = QString("Book %1 successfully.").arg(actionLabel);

            emit databaseUpdated("book");
            emit logProduced(QString("[ADMIN] Book ID %1 was %2.").arg(bookId).arg(actionLabel));
            QJsonObject broadcastObj;
            if (status == 1) {
                Book b;
                QString fetchErr;
                if (DatabaseManager::instance().fetchBook(bookId, b, fetchErr)) {
                    broadcastObj["action"] = "notify_book_updated";
                    broadcastObj["bookId"] = b.id;
                    broadcastObj["title"] = b.title;
                    broadcastObj["author"] = b.author;
                    broadcastObj["genre"] = b.genre;
                    broadcastObj["price"] = b.price;
                    broadcastObj["coverImagePath"] = b.coverImagePath;
                    broadcastObj["status"] = 1;
                }
            } else {
                broadcastObj["action"] = "notify_book_removed";
                broadcastObj["bookId"] = bookId;
            }

            emit broadcastTargetedUpdate(broadcastObj);
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }

    // else if (action == "publisher_set_book_status") {
    //     int bookId = requestObj["bookId"].toInt();
    //     int publisherId = requestObj["publisherId"].toInt();
    //     int status = requestObj["status"].toInt();

    //     if (status != 0 && status != 1) {
    //         responseObj["type"] = "action_result";
    //         responseObj["success"] = false;
    //         responseObj["message"] = "Publishers can only set active/inactive.";
    //     }
    //     else {
    //         Book b;
    //         QString fetchErr;
    //         if (!DatabaseManager::instance().fetchBook(bookId, b, fetchErr) || b.publisherId != publisherId) {
    //         } else {
    //             QString errorMsg;
    //             bool ok = DatabaseManager::instance().setBookStatus(bookId, status, errorMsg);
    //             responseObj["type"] = "action_result";
    //             responseObj["success"] = ok;
    //             responseObj["message"] = ok ? "Book status updated." : errorMsg;
    //             emit databaseUpdated("book");
    //             if (ok) {
    //                 QJsonObject broadcastObj;
    //                 if (status == 1) {
    //                     broadcastObj["action"] = "notify_book_updated";
    //                     broadcastObj["bookId"] = b.id;
    //                     broadcastObj["title"] = b.title;
    //                     broadcastObj["author"] = b.author;
    //                     broadcastObj["genre"] = b.genre;
    //                     broadcastObj["price"] = b.price;
    //                     broadcastObj["coverImagePath"] = b.coverImagePath;
    //                     broadcastObj["isActive"] = true;
    //                 } else { // غیرفعال شده
    //                     broadcastObj["action"] = "notify_book_removed";
    //                     broadcastObj["bookId"] = bookId;
    //                 }
    //                 emit broadcastTargetedUpdate(broadcastObj);
    //             }
    //         }
    //     }
    // }

    else if (action == "publisher_set_book_status") {
        int bookId = requestObj["bookId"].toInt();
        int publisherId = requestObj["publisherId"].toInt();
        int status = requestObj["status"].toInt();

        if (status != 0 && status != 1) {
            responseObj["type"] = "action_result";
            responseObj["success"] = false;
            responseObj["message"] = "Publishers can only set active/inactive.";
        }
        else {
            Book b;
            QString fetchErr;
            if (!DatabaseManager::instance().fetchBook(bookId, b, fetchErr) || b.publisherId != publisherId) {
                responseObj["type"] = "action_result";
                responseObj["success"] = false;
                responseObj["message"] = "Book not found or access denied.";
            } else {
                QString errorMsg;
                bool ok = DatabaseManager::instance().setBookStatus(bookId, status, errorMsg);
                responseObj["type"] = "action_result";
                responseObj["success"] = ok;
                responseObj["message"] = ok ? "Book status updated." : errorMsg;
                emit databaseUpdated("book");

                if (ok) {
                    QJsonObject broadcastObj;
                    if (status == 1) {
                        broadcastObj["action"] = "notify_book_updated";
                        broadcastObj["bookId"] = b.id;
                        broadcastObj["title"] = b.title;
                        broadcastObj["author"] = b.author;
                        broadcastObj["genre"] = b.genre;
                        broadcastObj["price"] = b.price;
                        broadcastObj["coverImagePath"] = b.coverImagePath;
                        broadcastObj["status"] = 1;
                    } else {
                        broadcastObj["action"] = "notify_book_removed";
                        broadcastObj["bookId"] = bookId;
                    }
                    emit broadcastTargetedUpdate(broadcastObj);
                }
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
    }
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
                o["isActive"] = b.status;
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
        return false;
    }
    return true;
}

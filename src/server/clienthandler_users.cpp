#include <QJsonArray>
#include "databasemanager.h"
#include "servermanager.h"
#include "clienthandler.h"

bool ClientHandler::handleUserActions(const QString &action, const QJsonObject &requestObj, QJsonObject &responseObj)
{
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
    else if (action == "delete_account") {
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
    }
    else if (action == "user_get_favorite_genres") {
        int userId = requestObj["userId"].toInt();
        QStringList genres;
        QString errorMsg;
        responseObj["type"] = "favorite_genres";
        if (DatabaseManager::instance().fetchUserFavoriteGenres(userId, genres, errorMsg)) {
            responseObj["success"] = true;
            responseObj["genres"] = QJsonArray::fromStringList(genres);
        } else {
            responseObj["success"] = false;
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "user_set_favorite_genres") {
        int userId = requestObj["userId"].toInt();
        QStringList genres;
        for (const QJsonValue &v : requestObj["genres"].toArray())
            genres << v.toString();
        QString errorMsg;
        responseObj["type"] = "favorite_genres_saved";
        if (DatabaseManager::instance().setUserFavoriteGenres(userId, genres, errorMsg)) {
            responseObj["success"] = true;
        } else {
            responseObj["success"] = false;
            responseObj["message"] = errorMsg;
        }
    }
    else {
        return false;
    }
    return true;
}

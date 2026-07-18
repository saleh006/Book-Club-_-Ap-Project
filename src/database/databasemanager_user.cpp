#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>
#include <QDate>

bool DatabaseManager::registerUser(const QString &username,
                                   const QString &password,
                                   const QString &fullName,
                                   const QString &email,
                                   const QString &recoveryAnswer,
                                   QString &errorMsg, const QString &role)
{
    if (username.trimmed().isEmpty() || password.isEmpty()) {
        errorMsg = "Username and password cannot be empty.";
        return false;
    }
    if (password.length() < 6) {
        errorMsg = "Password must be at least 6 characters.";
        return false;
    }
    QSqlQuery check(database());
    check.prepare("SELECT id FROM users WHERE username = :username");
    check.bindValue(":username", username);
    if (!check.exec()) {
        errorMsg = "Database error while checking username: " + check.lastError().text();
        return false;
    }
    if (check.next()) {
        errorMsg = "That username is already taken.";
        return false;
    }

    const QString salt = generateSalt();
    const QString hashedPass = hashPassword(password , salt);
    QString recoveryHash;
    QString recoverySalt;
    if (!recoveryAnswer.trimmed().isEmpty()) {
        recoverySalt = generateSalt();
        recoveryHash = hashPassword(recoveryAnswer.trimmed().toLower(), recoverySalt);
    } // recovery answer saved hashed

    QSqlQuery insert(database());
    insert.prepare(R"(
        INSERT INTO users
            (username, password_hash, salt, full_name, email,
             is_blocked, register_date, role, recovery_answer, recovery_salt)
        VALUES
            (:username, :hash, :salt, :fullName, :email,
             0, :registerDate, :role, :recoveryHash, :recoverySalt)
    )");
    insert.bindValue(":username", username);
    insert.bindValue(":hash", hashedPass);
    insert.bindValue(":salt", salt);
    insert.bindValue(":fullName", fullName);
    insert.bindValue(":email", email);
    insert.bindValue(":registerDate", QDate::currentDate().toString(Qt::ISODate));
    insert.bindValue(":role", role.isEmpty() ? "user" : role);
    insert.bindValue(":recoveryHash", recoveryHash);
    insert.bindValue(":recoverySalt", recoverySalt);
    if (!insert.exec()) {
        errorMsg = "Failed to create account: " + insert.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::authenticateUser(const QString &username,
                                       const QString &password,
                                       QString &errorMsg,
                                       User *outUser)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT id, username, password_hash, salt, full_name, email,
               is_blocked, register_date, role
        FROM users WHERE username = :username
    )");
    query.bindValue(":username", username);
    if (!query.exec()) {
        errorMsg = "Database error while logging in: " + query.lastError().text();
        return false;
    }
    if (!query.next()) {
        errorMsg = "No account found with that username.";
        return false;
    }
    const QString storedHash = query.value("password_hash").toString();
    const QString salt = query.value("salt").toString();
    const QString attemptedHash = hashPassword(password, salt);
    if (attemptedHash != storedHash) {
        errorMsg = "Incorrect password.";
        return false;
    }
    if (query.value("is_blocked").toBool()) {
        errorMsg = "This account has been blocked. Please contact support.";
        return false;
    }
    if (outUser) {
        outUser->id = query.value("id").toInt();
        outUser->username = query.value("username").toString();
        outUser->passwordHash = storedHash;
        outUser->fullName = query.value("full_name").toString();
        outUser->email = query.value("email").toString();
        outUser->isBlocked = false;
        outUser->registerDate = QDate::fromString(query.value("register_date").toString(), Qt::ISODate);
        outUser->role = query.value("role").toString();
    }
    return true;
}

bool DatabaseManager::setUserBlocked(const QString &username, bool blocked, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("UPDATE users SET is_blocked = :blocked WHERE username = :username");
    query.bindValue(":blocked", blocked ? 1 : 0);
    query.bindValue(":username", username);
    if (!query.exec()) {
        errorMsg = "Failed to update block status: " + query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        errorMsg = "No account found with that username.";
        return false;
    }
    return true;
}

bool DatabaseManager::deleteUser(const QString &username, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("DELETE FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (!query.exec()) {
        errorMsg = "Database error while deleting user: " + query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        errorMsg = "User not found.";
        return false;
    }
    return true;
}

bool DatabaseManager::resetPasswordWithRecovery(const QString &username, const QString &recoveryAnswer, const QString &newPassword, QString &errorMsg)
{
    if (newPassword.length() < 6) {
        errorMsg = "New password must be at least 6 characters.";
        return false;
    }
    QSqlQuery query(database());
    query.prepare("SELECT recovery_answer, recovery_salt FROM users WHERE username = :username");
    query.bindValue(":username", username);
    if (!query.exec()) {
        errorMsg = "Database error: " + query.lastError().text();
        return false;
    }
    if (!query.next()) {
        errorMsg = "No account found with that username.";
        return false;
    }
    const QString storedHash = query.value("recovery_answer").toString();
    const QString recoverySalt = query.value("recovery_salt").toString();
    if (storedHash.isEmpty() || recoverySalt.isEmpty()) {
        errorMsg = "No recovery answer was set for this account.";
        return false;
    }
    const QString attemptedHash = hashPassword(recoveryAnswer.trimmed().toLower(), recoverySalt);
    if (attemptedHash != storedHash) {
        errorMsg = "Recovery answer is incorrect.";
        return false;
    }
    const QString newSalt = generateSalt();
    const QString newHash = hashPassword(newPassword, newSalt);
    QSqlQuery update(database());
    update.prepare("UPDATE users SET password_hash = :hash, salt = :salt WHERE username = :username");
    update.bindValue(":hash", newHash);
    update.bindValue(":salt", newSalt);
    update.bindValue(":username", username);
    if (!update.exec()) {
        errorMsg = "Failed to update password: " + update.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::fetchUserProfileForAdmin(const QString &username, UserProfileSummary &outProfile, QString &errorMsg)
{
    if (!fetchUser(username, outProfile.user, errorMsg)) {
        return false; // allready has erorr msg
    }
    const int userId = outProfile.user.id;
    QString stepError;
    if (!fetchOwnedBooks(userId, outProfile.ownedBooks, stepError))
        qWarning() << "Failed to fetch owned books for admin view:" << stepError;
    if (!fetchWishlist(userId, outProfile.wishlist, stepError))
        qWarning() << "Failed to fetch wishlist for admin view:" << stepError;
    if (!fetchCart(userId, outProfile.cartItems, outProfile.cartTotal, stepError))
        qWarning() << "Failed to fetch cart for admin view:" << stepError;
    if (!fetchPurchaseHistory(userId, outProfile.purchaseHistory, stepError))
        qWarning() << "Failed to fetch purchase history for admin view:" << stepError;
    return true;
}

bool DatabaseManager::fetchAllUsersProfilesForAdmin(QVector<UserProfileSummary> &outProfiles, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("SELECT username FROM users WHERE role = 'user' ORDER BY id ASC");
    if (!query.exec()) {
        errorMsg = "Database error while listing users: " + query.lastError().text();
        return false;
    }
    QStringList usernames;
    while(query.next()) {
        usernames.push_back(query.value("username").toString());
    }
    outProfiles.clear();
    for (const QString& un : std::as_const(usernames)) {
        UserProfileSummary p;
        QString pErorr;
        if (fetchUserProfileForAdmin(un , p , pErorr)) {
            outProfiles.push_back(p);
        } else {
            qWarning() << "Skipping user" << un << "in admin listing:" << pErorr;
        }
    }
    return true;
}

QString DatabaseManager::generateSalt() const
{
    QByteArray bytes(16, 0);
    for (int i = 0; i < bytes.size(); ++i)
        bytes[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
    return QString::fromLatin1(bytes.toHex());
}

QString DatabaseManager::hashPassword(const QString &password, const QString &salt) const
{
    const QByteArray combined = (password + salt).toUtf8();
    return QString::fromLatin1(
        QCryptographicHash::hash(combined, QCryptographicHash::Sha256).toHex());
}

bool DatabaseManager::fetchUser(const QString &username, User &outUser, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT id, username, password_hash, full_name, email,
               is_blocked, register_date, role
        FROM users WHERE username = :username
    )");
    query.bindValue(":username", username);

    if (!query.exec()) {
        errorMsg = "Database error while fetching user: " + query.lastError().text();
        return false;
    }
    if (!query.next()) {
        errorMsg = "No account found with that username.";
        return false;
    }
    outUser.id = query.value("id").toInt();
    outUser.username = query.value("username").toString();
    outUser.passwordHash = query.value("password_hash").toString();
    outUser.fullName = query.value("full_name").toString();
    outUser.email = query.value("email").toString();
    outUser.isBlocked = query.value("is_blocked").toBool();
    outUser.registerDate = QDate::fromString(query.value("register_date").toString(), Qt::ISODate);
    outUser.role = query.value("role").toString();

    return true;
}

bool DatabaseManager::updateUserProfile(int userId, const QString &newUsername,
                                        const QString &fullName, const QString &email,
                                        QString &errorMsg)
{
    const QString newU = newUsername.trimmed();
    QSqlDatabase db = database();

    // username taken by anyone who is NOT me?
    QSqlQuery check(db);
    check.prepare(R"(SELECT id FROM users
                     WHERE username = :newU COLLATE NOCASE AND id <> :id)");
    check.bindValue(":newU", newU);
    check.bindValue(":id", userId);
    if (!check.exec()) { errorMsg = "Database error: " + check.lastError().text(); return false; }
    if (check.next())  { errorMsg = "This username is already taken."; return false; }

    QSqlQuery query(db);
    query.prepare(R"(UPDATE users SET username = :newU, full_name = :name, email = :email
                     WHERE id = :id)");
    query.bindValue(":newU", newU);
    query.bindValue(":name", fullName.trimmed());
    query.bindValue(":email", email.trimmed());
    query.bindValue(":id", userId);
    if (!query.exec()) { errorMsg = "Failed to update profile: " + query.lastError().text(); return false; }
    if (query.numRowsAffected() == 0) { errorMsg = "User not found."; return false; }
    return true;
}

bool DatabaseManager::changePassword(int userId, const QString &oldPassword,
                                     const QString &newPassword, QString &errorMsg)
{
    if (newPassword.length() < 6) { errorMsg = "New password must be at least 6 characters."; return false; }

    QSqlDatabase db = database();
    QSqlQuery query(db);
    query.prepare("SELECT password_hash, salt FROM users WHERE id = :id");
    query.bindValue(":id", userId);
    if (!query.exec()) { errorMsg = "Database error: " + query.lastError().text(); return false; }
    if (!query.next()) { errorMsg = "User not found."; return false; }

    if (hashPassword(oldPassword, query.value("salt").toString())
        != query.value("password_hash").toString()) {
        errorMsg = "Current password is incorrect.";
        return false;
    }

    const QString newSalt = generateSalt();
    QSqlQuery update(db);
    update.prepare("UPDATE users SET password_hash = :hash, salt = :salt WHERE id = :id");
    update.bindValue(":hash", hashPassword(newPassword, newSalt));
    update.bindValue(":salt", newSalt);
    update.bindValue(":id", userId);
    if (!update.exec()) { errorMsg = "Failed to change password: " + update.lastError().text(); return false; }
    return true;
}

bool DatabaseManager::setUserFavoriteGenres(int userId, const QStringList &genres, QString &errorMsg)
{
    QSqlDatabase db = database();
    if (!db.transaction()) {
        errorMsg = "Failed to start transaction: " + db.lastError().text();
        return false;
    }
    QSqlQuery del(db);
    del.prepare("DELETE FROM user_favorite_genres WHERE user_id = :uid");
    del.bindValue(":uid", userId);
    if (!del.exec()) {
        errorMsg = "Failed to clear old genres: " + del.lastError().text();
        db.rollback();
        return false;
    }
    for (const QString &g : genres) {
        QSqlQuery ins(db);
        ins.prepare("INSERT INTO user_favorite_genres (user_id, genre) VALUES (:uid, :genre)");
        ins.bindValue(":uid", userId);
        ins.bindValue(":genre", g);
        if (!ins.exec()) {
            errorMsg = "Failed to save genre: " + ins.lastError().text();
            db.rollback();
            return false;
        }
    }
    if (!db.commit()) {
        errorMsg = "Failed to save genres: " + db.lastError().text();
        db.rollback();
        return false;
    }
    return true;
}

bool DatabaseManager::fetchUserFavoriteGenres(int userId, QStringList &outGenres, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("SELECT genre FROM user_favorite_genres WHERE user_id = :uid");
    query.bindValue(":uid", userId);
    if (!query.exec()) {
        errorMsg = "Failed to fetch favorite genres: " + query.lastError().text();
        return false;
    }
    outGenres.clear();
    while (query.next())
        outGenres << query.value(0).toString();
    return true;
}
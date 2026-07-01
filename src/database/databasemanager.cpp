#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>



DatabaseManager &DatabaseManager::instance()
{
    static DatabaseManager mgr;
    return mgr;
}

bool DatabaseManager::initialize(const QString &dbPath)
{
    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        m_db = QSqlDatabase::database("qt_sql_default_connection");
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE");
        m_db.setDatabaseName(dbPath);
    }
    if (!m_db.isOpen() && !m_db.open()) {
        qWarning() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }
    return createTableForUser();
    // we should add all createTables in here !
}

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
    QSqlQuery check(m_db);
    check.prepare("SELECT id FROM users WHERE username = :username");
    check.bindValue(":username", username);
    if (!check.exec()) {
        errorMsg = "Database error while checking username.";
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
    QSqlQuery insert(m_db);
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
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, username, password_hash, salt, full_name, email,
               is_blocked, register_date, role
        FROM users WHERE username = :username
    )");
    query.bindValue(":username", username);
    if (!query.exec()) {
        errorMsg = "Database error while logging in.";
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
    QSqlQuery query(m_db);
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
// this function is for admin pannel
bool DatabaseManager::addBook(const Book &book, int &newBookId, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO books
            (title, author, genre, description, price,
             cover_image_path, pdf_path, is_active)
        VALUES
            (:title, :author, :genre, :description, :price,
             :cover, :pdf, :isActive)
    )");
    query.bindValue(":title", book.title);
    query.bindValue(":author", book.author);
    query.bindValue(":genre", book.genre);
    query.bindValue(":description", book.description);
    query.bindValue(":price", book.price);
    query.bindValue(":cover", book.coverImagePath);
    query.bindValue(":pdf", book.pdfPath);
    query.bindValue(":isActive", book.isActive ? 1 : 0);
    if (!query.exec()) {
        errorMsg = "Failed to add book: " + query.lastError().text();
        return false;
    }
    newBookId = query.lastInsertId().toInt();
    return true;
}

bool DatabaseManager::updateBook(const Book &book, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE books SET
            title = :title, author = :author, genre = :genre,
            description = :description, price = :price,
            cover_image_path = :cover, pdf_path = :pdf,
            is_active = :isActive
        WHERE id = :id
    )");
    query.bindValue(":title", book.title);
    query.bindValue(":author", book.author);
    query.bindValue(":genre", book.genre);
    query.bindValue(":description", book.description);
    query.bindValue(":price", book.price);
    query.bindValue(":cover", book.coverImagePath);
    query.bindValue(":pdf", book.pdfPath);
    query.bindValue(":isActive", book.isActive ? 1 : 0);
    query.bindValue(":id", book.id);
    if (!query.exec()) {
        errorMsg = "Failed to update book: " + query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::deleteBook(int bookId, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE books SET is_active = 0 WHERE id = :id");
    query.bindValue(":id", bookId);
    if (!query.exec()) {
        errorMsg = "Failed to remove book: " + query.lastError().text();
        return false;
    }
    return true;
} // soft delete keeps purchase/review history intact

bool DatabaseManager::fetchBook(int bookId, Book &outBook, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM books WHERE id = :id");
    query.bindValue(":id", bookId);
    if (!query.exec()) {
        errorMsg = "Database error while fetching book.";
        return false;
    }
    if (!query.next()) {
        errorMsg = "Book not found.";
        return false;
    }
    outBook.id = query.value("id").toInt();
    outBook.title = query.value("title").toString();
    outBook.author = query.value("author").toString();
    outBook.genre = query.value("genre").toString();
    outBook.description = query.value("description").toString();
    outBook.price = query.value("price").toDouble();
    outBook.coverImagePath = query.value("cover_image_path").toString();
    outBook.pdfPath = query.value("pdf_path").toString();
    outBook.isActive = query.value("is_active").toBool();
    outBook.averageRating = query.value("average_rating").toDouble();
    outBook.totalSales = query.value("total_sales").toInt();
    return true;
}

bool DatabaseManager::fetchAllBooks(QVector<Book> &outBooks, QString &errorMsg, bool activeOnly)
{
    QSqlQuery query(m_db);
    query.prepare(activeOnly ? "SELECT * FROM books WHERE is_active = 1"
                             : "SELECT * FROM books");
    if (!query.exec()) {
        errorMsg = "Database error while fetching books.";
        return false;
    }
    outBooks.clear();
    while (query.next()) {
        Book b;
        b.id = query.value("id").toInt();
        b.title = query.value("title").toString();
        b.author = query.value("author").toString();
        b.genre = query.value("genre").toString();
        b.description = query.value("description").toString();
        b.price = query.value("price").toDouble();
        b.coverImagePath = query.value("cover_image_path").toString();
        b.pdfPath = query.value("pdf_path").toString();
        b.isActive = query.value("is_active").toBool();
        b.averageRating = query.value("average_rating").toDouble();
        b.totalSales = query.value("total_sales").toInt();
        outBooks.push_back(b);
    }
    return true;
}

bool DatabaseManager::fetchBooksByGenre(const QString &genre, QVector<Book> &outBooks, QString &errorMsg)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM books WHERE genre = :genre AND is_active = 1");
    query.bindValue(":genre", genre);
    if (!query.exec()) {
        errorMsg = "Database error while fetching books by genre.";
        return false;
    }
    outBooks.clear();
    while (query.next()) {
        Book b;
        b.id = query.value("id").toInt();
        b.title = query.value("title").toString();
        b.author = query.value("author").toString();
        b.genre = query.value("genre").toString();
        b.price = query.value("price").toDouble();
        b.coverImagePath = query.value("cover_image_path").toString();
        outBooks.push_back(b);
    }
    return true;
}














bool DatabaseManager::createTableForUser()
{
    QSqlQuery query(m_db);
    const QString sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id               INTEGER PRIMARY KEY AUTOINCREMENT,
            username         TEXT UNIQUE NOT NULL,
            password_hash    TEXT NOT NULL,
            salt             TEXT NOT NULL,
            full_name        TEXT,
            email            TEXT,
            is_blocked       INTEGER NOT NULL DEFAULT 0,
            register_date    DATE DEFAULT (date('now')),
            role             TEXT NOT NULL DEFAULT 'user',
            recovery_answer  TEXT,
            recovery_salt    TEXT,
            created_at       DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    if (!query.exec(sql)) {
        qWarning() << "Failed to create users table:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::createTableForBooks()
{
    QSqlQuery query(m_db);
    const QString sql = R"(
        CREATE TABLE IF NOT EXISTS books (
            id               INTEGER PRIMARY KEY AUTOINCREMENT,
            publisher_id     INTEGER NOT NULL,
            title            TEXT NOT NULL,
            author           TEXT,
            genre            TEXT,
            description      TEXT,
            price            REAL NOT NULL DEFAULT 0,
            cover_image_path TEXT,
            pdf_path         TEXT,
            is_active        INTEGER NOT NULL DEFAULT 1,
            average_rating   REAL NOT NULL DEFAULT 0,
            total_sales      INTEGER NOT NULL DEFAULT 0,
            created_at       DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (publisher_id) REFERENCES users(id)
        )
    )";
    if (!query.exec(sql)) {
        qWarning() << "Failed to create books table:" << query.lastError().text();
        return false;
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
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, username, password_hash, full_name, email,
               is_blocked, register_date, role
        FROM users WHERE username = :username
    )");
    query.bindValue(":username", username);

    if (!query.exec()) {
        errorMsg = "Database error while fetching user.";
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
} // this for admin panel











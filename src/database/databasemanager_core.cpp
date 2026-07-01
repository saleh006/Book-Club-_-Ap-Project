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

bool DatabaseManager::createTableForDiscounts()
{
    QSqlQuery query(m_db);
    const QString sql = R"(
    CREATE TABLE IF NOT EXISTS discounts (
        id          INTEGER PRIMARY KEY AUTOINCREMENT,
        book_id     INTEGER NOT NULL,
        type        TEXT NOT NULL,
        value       REAL NOT NULL,
        start_date  DATETIME,
        end_date    DATETIME,
        FOREIGN KEY (book_id) REFERENCES books(id)
    )
    )";
    if (!query.exec(sql)) {
        qWarning() << "Failed to create discounts table:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::createTableForShelves()
{
    QSqlQuery query(m_db);
    const QString sql1 = R"(
        CREATE TABLE IF NOT EXISTS shelves (
            id       INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id  INTEGER NOT NULL,
            title    TEXT NOT NULL,
            FOREIGN KEY (user_id) REFERENCES users(id)
        )
    )";
    if (!query.exec(sql1)) {
        qWarning() << "Failed to create shelves table:" << query.lastError().text();
        return false;
    }
    const QString sql2 = R"(
        CREATE TABLE IF NOT EXISTS shelf_books (
            shelf_id  INTEGER NOT NULL,
            book_id   INTEGER NOT NULL,
            PRIMARY KEY (shelf_id, book_id),
            FOREIGN KEY (shelf_id) REFERENCES shelves(id),
            FOREIGN KEY (book_id) REFERENCES books(id)
        )
    )";
    if (!query.exec(sql2)) {
        qWarning() << "Failed to create shelf_books table:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::createTableForReadingProgress()
{
    QSqlQuery query(m_db);
    const QString sql = R"(
        CREATE TABLE IF NOT EXISTS reading_progress (
            user_id    INTEGER NOT NULL,
            book_id    INTEGER NOT NULL,
            last_page  INTEGER NOT NULL DEFAULT 0,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY (user_id, book_id),
            FOREIGN KEY (user_id) REFERENCES users(id),
            FOREIGN KEY (book_id) REFERENCES books(id)
        )
    )";
    if (!query.exec(sql)) {
        qWarning() << "Failed to create reading_progress table:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::createTableForReviews()
{
    QSqlQuery query(m_db);
    const QString sql = R"(
        CREATE TABLE IF NOT EXISTS reviews (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id   INTEGER NOT NULL,
            book_id   INTEGER NOT NULL,
            comment   TEXT,
            rating    INTEGER NOT NULL CHECK (rating BETWEEN 1 AND 5),
            date      DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES users(id),
            FOREIGN KEY (book_id) REFERENCES books(id)
        )
    )";
    if (!query.exec(sql)) {
        qWarning() << "Failed to create reviews table:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::createTableForNotifications()
{
    QSqlQuery query(m_db);
    const QString sql = R"(
        CREATE TABLE IF NOT EXISTS notifications (
            id       INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id  INTEGER NOT NULL,
            title    TEXT NOT NULL,
            message  TEXT,
            date     DATETIME DEFAULT CURRENT_TIMESTAMP,
            is_read  INTEGER NOT NULL DEFAULT 0,
            FOREIGN KEY (user_id) REFERENCES users(id)
        )
    )";
    if (!query.exec(sql)) {
        qWarning() << "Failed to create notifications table:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::createTableForCart()
{
    QSqlQuery query(m_db);
    const QString sql = R"(
        CREATE TABLE IF NOT EXISTS cart_items (
            user_id    INTEGER NOT NULL,
            book_id    INTEGER NOT NULL,
            quantity   INTEGER NOT NULL DEFAULT 1,
            added_at   DATETIME DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY (user_id, book_id),
            FOREIGN KEY (user_id) REFERENCES users(id),
            FOREIGN KEY (book_id) REFERENCES books(id)
        )
    )";
    if (!query.exec(sql)) {
        qWarning() << "Failed to create cart_items table:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::createTableForPurchases()
{
    QSqlQuery query(m_db);
    const QString sql1 = R"(
        CREATE TABLE IF NOT EXISTS purchases (
            id             INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id        INTEGER NOT NULL,
            total_price    REAL NOT NULL DEFAULT 0,
            purchase_date  DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES users(id)
        )
    )";
    if (!query.exec(sql1)) {
        qWarning() << "Failed to create purchases table:" << query.lastError().text();
        return false;
    }
    const QString sql2 = R"(
        CREATE TABLE IF NOT EXISTS purchase_items (
            purchase_id  INTEGER NOT NULL,
            book_id      INTEGER NOT NULL,
            quantity     INTEGER NOT NULL DEFAULT 1,
            price_paid   REAL NOT NULL DEFAULT 0,
            PRIMARY KEY (purchase_id, book_id),
            FOREIGN KEY (purchase_id) REFERENCES purchases(id),
            FOREIGN KEY (book_id) REFERENCES books(id)
        )
    )";
    if (!query.exec(sql2)) {
        qWarning() << "Failed to create purchase_items table:" << query.lastError().text();
        return false;
    }
    return true;
}


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
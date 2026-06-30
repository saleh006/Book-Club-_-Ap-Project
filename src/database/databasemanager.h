#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H
#include <QSqlDatabase>
#include <QString>
#include <QDate>

struct User
{
    int id = -1;
    QString username;
    QString passwordHash;
    QString fullName;
    QString email;
    bool isBlocked = false;
    QDate registerDate;
    QString role;
    QString recoveryAnswer;
};

class DatabaseManager
{
public:
    static DatabaseManager &instance();

    bool initialize(const QString &dbPath);

    bool registerUser(const QString &username,
                      const QString &password,
                      const QString &fullName,
                      const QString &email,
                      const QString &recoveryAnswer,
                      QString &errorMsg,
                      const QString &role = "user");

    bool authenticateUser(const QString &username,
                          const QString &password,
                          QString &errorMsg,
                          User *outUser = nullptr);

    bool fetchUser(const QString &username, User &outUser, QString &errorMsg);

    bool setUserBlocked(const QString &username, bool blocked, QString &errorMsg);

private:
    DatabaseManager() = default;
    bool createTableForUser();
    QString generateSalt() const;
    QString hashPassword(const QString &password, const QString &salt) const;

    QSqlDatabase m_db;
};

#endif // DATABASEMANAGER_H

#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

bool DatabaseManager::createStudyRoom(int bookId, int creatorId, const QString &name, int &newRoomId, QString &errorMsg)
{
    if (name.trimmed().isEmpty()) {
        errorMsg = "Study room name cannot be empty.";
        return false;
    }

    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO study_rooms (book_id, creator_id, name)
        VALUES (:bookId, :creatorId, :name)
    )");
    query.bindValue(":bookId", bookId);
    query.bindValue(":creatorId", creatorId);
    query.bindValue(":name", name.trimmed());

    if (!query.exec()) {
        errorMsg = "Failed to create study room: " + query.lastError().text();
        return false;
    }
    newRoomId = query.lastInsertId().toInt();

    // Creator automatically becomes the first member
    QSqlQuery memberQuery(database());
    memberQuery.prepare("INSERT INTO study_room_members (room_id, user_id) VALUES (:roomId, :userId)");
    memberQuery.bindValue(":roomId", newRoomId);
    memberQuery.bindValue(":userId", creatorId);
    if (!memberQuery.exec()) {
        errorMsg = "Room created but failed to add you as a member: " + memberQuery.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::fetchStudyRoomsForBook(int bookId, QVector<StudyRoom> &outRooms, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT sr.id, sr.book_id, sr.creator_id, sr.name, sr.created_at, u.username AS creator_username,
               (SELECT COUNT(*) FROM study_room_members m WHERE m.room_id = sr.id) AS member_count
        FROM study_rooms sr
        JOIN users u ON u.id = sr.creator_id
        WHERE sr.book_id = :bookId AND sr.is_active = 1
        ORDER BY sr.created_at DESC
    )");
    query.bindValue(":bookId", bookId);

    if (!query.exec()) {
        errorMsg = "Database error while fetching study rooms: " + query.lastError().text();
        return false;
    }

    outRooms.clear();
    while (query.next()) {
        StudyRoom r;
        r.id = query.value("id").toInt();
        r.bookId = query.value("book_id").toInt();
        r.creatorId = query.value("creator_id").toInt();
        r.creatorUsername = query.value("creator_username").toString();
        r.name = query.value("name").toString();
        r.createdAt = query.value("created_at").toDateTime();
        r.memberCount = query.value("member_count").toInt();
        outRooms.push_back(r);
    }
    return true;
}

bool DatabaseManager::fetchStudyRoom(int roomId, StudyRoom &outRoom, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT sr.id, sr.book_id, sr.creator_id, sr.name, sr.created_at, sr.is_active, u.username AS creator_username
        FROM study_rooms sr
        JOIN users u ON u.id = sr.creator_id
        WHERE sr.id = :id
    )");
    query.bindValue(":id", roomId);

    if (!query.exec()) {
        errorMsg = "Database error while fetching study room: " + query.lastError().text();
        return false;
    }
    if (!query.next()) {
        errorMsg = "Study room not found.";
        return false;
    }
    if (query.value("is_active").toInt() != 1) {
        errorMsg = "This study room has been closed.";
        return false;
    }

    outRoom.id = query.value("id").toInt();
    outRoom.bookId = query.value("book_id").toInt();
    outRoom.creatorId = query.value("creator_id").toInt();
    outRoom.creatorUsername = query.value("creator_username").toString();
    outRoom.name = query.value("name").toString();
    outRoom.createdAt = query.value("created_at").toDateTime();
    return true;
}

bool DatabaseManager::joinStudyRoom(int roomId, int userId, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("INSERT OR IGNORE INTO study_room_members (room_id, user_id) VALUES (:roomId, :userId)");
    query.bindValue(":roomId", roomId);
    query.bindValue(":userId", userId);

    if (!query.exec()) {
        errorMsg = "Failed to join study room: " + query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::leaveStudyRoom(int roomId, int userId, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("DELETE FROM study_room_members WHERE room_id = :roomId AND user_id = :userId");
    query.bindValue(":roomId", roomId);
    query.bindValue(":userId", userId);

    if (!query.exec()) {
        errorMsg = "Failed to leave study room: " + query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::fetchStudyRoomMembers(int roomId, QVector<StudyRoomMember> &outMembers, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT m.user_id, u.username, m.joined_at
        FROM study_room_members m
        JOIN users u ON u.id = m.user_id
        WHERE m.room_id = :roomId
        ORDER BY m.joined_at ASC
    )");
    query.bindValue(":roomId", roomId);

    if (!query.exec()) {
        errorMsg = "Database error while fetching study room members: " + query.lastError().text();
        return false;
    }

    outMembers.clear();
    while (query.next()) {
        StudyRoomMember m;
        m.userId = query.value("user_id").toInt();
        m.username = query.value("username").toString();
        m.joinedAt = query.value("joined_at").toDateTime();
        outMembers.push_back(m);
    }
    return true;
}

bool DatabaseManager::isStudyRoomMember(int roomId, int userId, bool &outIsMember, QString &errorMsg)
{
    QSqlQuery query(database());
    query.prepare("SELECT 1 FROM study_room_members WHERE room_id = :roomId AND user_id = :userId");
    query.bindValue(":roomId", roomId);
    query.bindValue(":userId", userId);

    if (!query.exec()) {
        errorMsg = "Database error while checking study room membership: " + query.lastError().text();
        return false;
    }
    outIsMember = query.next();
    return true;
}


bool DatabaseManager::closeStudyRoom(int roomId, int requesterId, QString &errorMsg)
{
    QSqlQuery checkQuery(database());
    checkQuery.prepare("SELECT creator_id FROM study_rooms WHERE id = :id");
    checkQuery.bindValue(":id", roomId);
    if (!checkQuery.exec() || !checkQuery.next()) {
        errorMsg = "Study room not found.";
        return false;
    }
    if (checkQuery.value("creator_id").toInt() != requesterId) {
        errorMsg = "Only the room creator can close this study room.";
        return false;
    }

    QSqlQuery query(database());
    query.prepare("UPDATE study_rooms SET is_active = 0 WHERE id = :id");
    query.bindValue(":id", roomId);
    if (!query.exec()) {
        errorMsg = "Failed to close study room: " + query.lastError().text();
        return false;
    }
    return true;
}
#include <QJsonArray>
#include <algorithm>
#include "databasemanager.h"
#include "clienthandler.h"

void ClientHandler::broadcastToStudyRoomMembers(int roomId, const QJsonObject &payload, int excludeUserId)
{
    QVector<StudyRoomMember> members;
    QString errorMsg;
    if (!DatabaseManager::instance().fetchStudyRoomMembers(roomId, members, errorMsg)) {
        qWarning() << "Failed to fetch study room members for broadcast:" << errorMsg;
        return;
    }
    for (const StudyRoomMember &m : members) {
        if (m.userId == excludeUserId) continue;
        emit studyRoomEventReady(m.userId, payload);
    }
}

bool ClientHandler::handleStudyRoomActions(const QString &action, const QJsonObject &requestObj, QJsonObject &responseObj)
{
    if (action == "studyroom_create") {
        responseObj["action"] = "studyroom_create_response";
        int userId = requestObj["userId"].toInt();
        int bookId = requestObj["bookId"].toInt();
        QString name = requestObj["name"].toString().trimmed();

        if (name.isEmpty()) {
            responseObj["status"] = "error";
            responseObj["message"] = "Please give your study room a name.";
            return true;
        }

        QVector<Book> owned;
        QString ownedErr;
        bool ownsBook = DatabaseManager::instance().fetchOwnedBooks(userId, owned, ownedErr) &&
                        std::any_of(owned.begin(), owned.end(), [bookId](const Book &b) { return b.id == bookId; });
        if (!ownsBook) {
            responseObj["status"] = "error";
            responseObj["message"] = "You must own this book to start a study room for it.";
            return true;
        }

        int newRoomId = -1;
        QString errorMsg;
        if (DatabaseManager::instance().createStudyRoom(bookId, userId, name, newRoomId, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Study room created.";
            responseObj["roomId"] = newRoomId;
            responseObj["bookId"] = bookId;
            responseObj["name"] = name;

            m_studyRoomMemberships.append(qMakePair(newRoomId, userId));
            emit logProduced(QString("[STUDY] Room \"%1\" (ID %2) created for book ID %3 by %4.")
                                 .arg(name).arg(newRoomId).arg(bookId).arg(m_username));
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "studyroom_list_for_book") {
        responseObj["action"] = "studyroom_list_response";
        int bookId = requestObj["bookId"].toInt();
        responseObj["bookId"] = bookId;

        QVector<StudyRoom> rooms;
        QString errorMsg;
        if (DatabaseManager::instance().fetchStudyRoomsForBook(bookId, rooms, errorMsg)) {
            responseObj["status"] = "success";
            QJsonArray arr;
            for (const StudyRoom &r : rooms) {
                QJsonObject ro;
                ro["id"] = r.id;
                ro["bookId"] = r.bookId;
                ro["name"] = r.name;
                ro["creatorId"] = r.creatorId;
                ro["creatorUsername"] = r.creatorUsername;
                ro["memberCount"] = r.memberCount;
                arr.append(ro);
            }
            responseObj["rooms"] = arr;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "studyroom_join") {
        responseObj["action"] = "studyroom_join_response";
        int userId = requestObj["userId"].toInt();
        int roomId = requestObj["roomId"].toInt();
        responseObj["roomId"] = roomId;

        StudyRoom room;
        QString roomErr;
        if (!DatabaseManager::instance().fetchStudyRoom(roomId, room, roomErr)) {
            responseObj["status"] = "error";
            responseObj["message"] = roomErr.isEmpty() ? "Study room not found." : roomErr;
            return true;
        }

        QVector<Book> owned;
        QString ownedErr;
        bool ownsBook = DatabaseManager::instance().fetchOwnedBooks(userId, owned, ownedErr) &&
                        std::any_of(owned.begin(), owned.end(), [&room](const Book &b) { return b.id == room.bookId; });
        if (!ownsBook) {
            responseObj["status"] = "error";
            responseObj["message"] = "You must own this book to join its study room.";
            return true;
        }

        QString errorMsg;
        if (DatabaseManager::instance().joinStudyRoom(roomId, userId, errorMsg)) {
            m_studyRoomMemberships.append(qMakePair(roomId, userId));

            QVector<StudyRoomMember> members;
            QString memErr;
            DatabaseManager::instance().fetchStudyRoomMembers(roomId, members, memErr);

            responseObj["status"] = "success";
            responseObj["bookId"] = room.bookId;
            responseObj["name"] = room.name;
            responseObj["creatorId"] = room.creatorId;

            QJsonArray memArr;
            for (const StudyRoomMember &m : members) {
                QJsonObject mo;
                mo["userId"] = m.userId;
                mo["username"] = m.username;
                memArr.append(mo);
            }

            QJsonObject notice;
            notice["type"] = "studyroom_member_joined";
            notice["roomId"] = roomId;
            notice["userId"] = userId;
            notice["username"] = m_username;
            broadcastToStudyRoomMembers(roomId, notice, userId);

            emit logProduced(QString("[STUDY] %1 joined room ID %2.").arg(m_username).arg(roomId));
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "studyroom_leave") {
        responseObj["action"] = "studyroom_leave_response";
        int userId = requestObj["userId"].toInt();
        int roomId = requestObj["roomId"].toInt();
        responseObj["roomId"] = roomId;

        QString errorMsg;
        if (DatabaseManager::instance().leaveStudyRoom(roomId, userId, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Left the study room.";

            m_studyRoomMemberships.erase(
                std::remove_if(m_studyRoomMemberships.begin(), m_studyRoomMemberships.end(),
                               [roomId, userId](const QPair<int,int> &p) { return p.first == roomId && p.second == userId; }),
                m_studyRoomMemberships.end());

            QJsonObject notice;
            notice["type"] = "studyroom_member_left";
            notice["roomId"] = roomId;
            notice["userId"] = userId;
            notice["username"] = m_username;
            broadcastToStudyRoomMembers(roomId, notice, userId);
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "studyroom_fetch_members") {
        responseObj["action"] = "studyroom_members_response";
        int roomId = requestObj["roomId"].toInt();
        responseObj["roomId"] = roomId;

        QVector<StudyRoomMember> members;
        QString errorMsg;
        if (DatabaseManager::instance().fetchStudyRoomMembers(roomId, members, errorMsg)) {
            responseObj["status"] = "success";
            QJsonArray arr;
            for (const StudyRoomMember &m : members) {
                QJsonObject mo;
                mo["userId"] = m.userId;
                mo["username"] = m.username;
                arr.append(mo);
            }
            responseObj["members"] = arr;
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "studyroom_close") {
        responseObj["action"] = "studyroom_close_response";
        int userId = requestObj["userId"].toInt();
        int roomId = requestObj["roomId"].toInt();
        responseObj["roomId"] = roomId;

        QString errorMsg;
        if (DatabaseManager::instance().closeStudyRoom(roomId, userId, errorMsg)) {
            responseObj["status"] = "success";
            responseObj["message"] = "Study room closed.";

            QJsonObject notice;
            notice["type"] = "studyroom_closed";
            notice["roomId"] = roomId;
            broadcastToStudyRoomMembers(roomId, notice, userId);

            m_studyRoomMemberships.erase(
                std::remove_if(m_studyRoomMemberships.begin(), m_studyRoomMemberships.end(),
                               [roomId](const QPair<int,int> &p) { return p.first == roomId; }),
                m_studyRoomMemberships.end());

            emit databaseUpdated("study_rooms");
            emit logProduced(QString("[STUDY] Room ID %1 was closed by %2.").arg(roomId).arg(m_username));
        } else {
            responseObj["status"] = "error";
            responseObj["message"] = errorMsg;
        }
    }
    else if (action == "studyroom_page_sync") {
        responseObj["action"] = "studyroom_page_sync_response";
        int userId = requestObj["userId"].toInt();
        int roomId = requestObj["roomId"].toInt();
        int page = requestObj["page"].toInt();
        responseObj["roomId"] = roomId;

        StudyRoom room;
        QString roomErr;
        if (!DatabaseManager::instance().fetchStudyRoom(roomId, room, roomErr)) {
            responseObj["status"] = "error";
            responseObj["message"] = roomErr.isEmpty() ? "Study room not found." : roomErr;
            return true;
        }
        if (room.creatorId != userId) {
            responseObj["status"] = "error";
            responseObj["message"] = "Only the room creator can control the shared reader.";
            return true;
        }

        responseObj["status"] = "success";

        QJsonObject notice;
        notice["type"] = "studyroom_page_sync";
        notice["roomId"] = roomId;
        notice["page"] = page;
        broadcastToStudyRoomMembers(roomId, notice, userId);
    }
    else {
        return false;
    }
    return true;
}
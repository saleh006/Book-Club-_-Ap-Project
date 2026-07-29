#ifndef STUDYROOMSPAGE_H
#define STUDYROOMSPAGE_H

#include <QWidget>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include "models.h"

class QComboBox;
class QLineEdit;
class QPushButton;
class QListWidget;
class QListWidgetItem;

class StudyRoomsPage : public QWidget
{
    Q_OBJECT
public:
    explicit StudyRoomsPage(QTcpSocket *socket, int userId, QWidget *parent = nullptr);

    void setOwnedBooks(const QVector<Book> &books);
    void handleServerResponse(const QJsonObject &responseObj);

signals:
    void studyRoomReaderRequested(int bookId, int roomId, bool isCreator, const QString &roomName);

private slots:
    void onBookSelectionChanged(int index);
    void onCreateClicked();
    void onRoomItemDoubleClicked(QListWidgetItem *item);

private:
    void setupUi();
    void sendRequest(const QJsonObject &requestObj);
    void requestRoomsForSelectedBook();
    void rebuildRoomList(const QJsonArray &rooms);
    void closeRoomDirect(int roomId);
    void joinRoom(int roomId);

    QTcpSocket *m_socket;
    int m_userId;

    QVector<Book> m_ownedBooks;
    QComboBox *m_bookSelector = nullptr;
    QLineEdit *m_newRoomNameEdit = nullptr;
    QPushButton *m_createBtn = nullptr;
    QListWidget *m_roomListWidget = nullptr;
};

#endif // STUDYROOMSPAGE_H
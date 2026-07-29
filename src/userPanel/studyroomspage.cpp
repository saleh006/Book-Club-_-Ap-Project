#include "studyroomspage.h"
#include "styledmessagebox.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QWidget>
#include <QJsonDocument>

StudyRoomsPage::StudyRoomsPage(QTcpSocket *socket, int userId, QWidget *parent)
    : QWidget(parent), m_socket(socket), m_userId(userId)
{
    setupUi();
}

void StudyRoomsPage::setupUi()
{
    setStyleSheet("background-color:#0B0810;border:none;");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(30, 26, 30, 30);
    layout->setSpacing(14);

    auto *title = new QLabel("Study Rooms", this);
    title->setStyleSheet("color:#FFFFFF;font-size:21px;font-weight:700;border:none;background:transparent;");
    auto *subtitle = new QLabel("Study together with other readers of the same book", this);
    subtitle->setStyleSheet("color:#9A8FA0;font-size:12px;border:none;background:transparent;");
    layout->addWidget(title);
    layout->addWidget(subtitle);

    auto *pickerRow = new QHBoxLayout();
    m_bookSelector = new QComboBox(this);
    m_bookSelector->setStyleSheet(
        "QComboBox{background:#120E14;color:#EAEAEA;border:1px solid #241A2E;border-radius:8px;padding:6px 10px;}");
    connect(m_bookSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StudyRoomsPage::onBookSelectionChanged);
    pickerRow->addWidget(m_bookSelector, 1);
    layout->addLayout(pickerRow);

    auto *createRow = new QHBoxLayout();
    m_newRoomNameEdit = new QLineEdit(this);
    m_newRoomNameEdit->setPlaceholderText("New room name (e.g. \"Chapter 5 discussion\")");
    m_newRoomNameEdit->setStyleSheet(
        "QLineEdit{background:#120E14;color:#F5F1F7;border:1px solid #241A2E;border-radius:8px;padding:8px 10px;}");

    m_createBtn = new QPushButton("+ Create Room", this);
    m_createBtn->setCursor(Qt::PointingHandCursor);
    m_createBtn->setStyleSheet(
        "QPushButton{background-color:#7C3E66;color:white;border:none;border-radius:8px;padding:8px 16px;font-weight:600;}"
        "QPushButton:hover{background-color:#93507B;}");
    connect(m_createBtn, &QPushButton::clicked, this, &StudyRoomsPage::onCreateClicked);

    createRow->addWidget(m_newRoomNameEdit, 1);
    createRow->addWidget(m_createBtn);
    layout->addLayout(createRow);

    m_roomListWidget = new QListWidget(this);
    m_roomListWidget->setFrameShape(QFrame::NoFrame);
    m_roomListWidget->setSpacing(6);
    m_roomListWidget->setStyleSheet(
        "QListWidget{background-color:transparent;border:none;color:#EAEAEA;}"
        "QListWidget::item{background:#120E14;border:1px solid #241A2E;border-radius:8px;padding:10px;}"
        "QListWidget::item:hover{border-color:#7C3E66;}");
    connect(m_roomListWidget, &QListWidget::itemDoubleClicked, this, &StudyRoomsPage::onRoomItemDoubleClicked);
    layout->addWidget(m_roomListWidget, 1);

    auto *hint = new QLabel("Click \"Open\" to start reading together. The room creator controls page navigation for everyone.", this);
    hint->setStyleSheet("color:#665A72;font-size:11px;border:none;background:transparent;");
    layout->addWidget(hint);
}

void StudyRoomsPage::sendRequest(const QJsonObject &requestObj)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    m_socket->write(QJsonDocument(requestObj).toJson(QJsonDocument::Compact) + "\n");
}

void StudyRoomsPage::setOwnedBooks(const QVector<Book> &books)
{
    int prevBookId = m_bookSelector->count() > 0 ? m_bookSelector->currentData().toInt() : -1;

    m_ownedBooks = books;
    m_bookSelector->blockSignals(true);
    m_bookSelector->clear();
    for (const Book &b : m_ownedBooks) {
        m_bookSelector->addItem(b.title, b.id);
    }
    m_bookSelector->blockSignals(false);

    if (m_ownedBooks.isEmpty()) {
        m_roomListWidget->clear();
        return;
    }

    int restoreIndex = m_bookSelector->findData(prevBookId);
    m_bookSelector->setCurrentIndex(restoreIndex >= 0 ? restoreIndex : 0);
    requestRoomsForSelectedBook();
}

void StudyRoomsPage::onBookSelectionChanged(int index)
{
    Q_UNUSED(index);
    requestRoomsForSelectedBook();
}

void StudyRoomsPage::requestRoomsForSelectedBook()
{
    int bookId = m_bookSelector->currentData().toInt();
    if (bookId <= 0) return;

    QJsonObject req;
    req["action"] = "studyroom_list_for_book";
    req["bookId"] = bookId;
    sendRequest(req);
}

void StudyRoomsPage::onCreateClicked()
{
    int bookId = m_bookSelector->currentData().toInt();
    QString name = m_newRoomNameEdit->text().trimmed();

    if (bookId <= 0) {
        StyledMessageBox::error(this, "No Book Selected", "You need to own at least one book to create a study room.");
        return;
    }
    if (name.isEmpty()) {
        StyledMessageBox::error(this, "Name Required", "Please give your study room a name.");
        return;
    }

    QJsonObject req;
    req["action"] = "studyroom_create";
    req["userId"] = m_userId;
    req["bookId"] = bookId;
    req["name"] = name;
    sendRequest(req);
}

void StudyRoomsPage::rebuildRoomList(const QJsonArray &rooms)
{
    m_roomListWidget->clear();

    for (const QJsonValue &v : rooms) {
        QJsonObject ro = v.toObject();
        int roomId = ro["id"].toInt();
        int creatorId = ro["creatorId"].toInt();
        int memberCount = ro["memberCount"].toInt();
        QString roomName = ro["name"].toString();
        QString creatorName = ro["creatorUsername"].toString();

        auto *item = new QListWidgetItem(m_roomListWidget);
        item->setData(Qt::UserRole, roomId);

        auto *card = new QWidget(m_roomListWidget);
        card->setStyleSheet("background:transparent;border:none;");
        card->setMinimumHeight(76);

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(4, 8, 4, 8);
        cardLayout->setSpacing(4);

        auto *nameLabel = new QLabel(roomName, card);
        nameLabel->setStyleSheet("color:#F5F1F7;font-size:14px;font-weight:700;background:transparent;border:none;");
        cardLayout->addWidget(nameLabel);

        auto *metaLabel = new QLabel(
            QString("%1 member%2  •  started by %3")
                .arg(memberCount)
                .arg(memberCount == 1 ? "" : "s")
                .arg(creatorName),
            card);
        metaLabel->setStyleSheet("color:#9A8FA0;font-size:12px;background:transparent;border:none;");
        cardLayout->addWidget(metaLabel);

        auto *btnRow = new QHBoxLayout();
        btnRow->setContentsMargins(0, 0, 0 , 0);
        btnRow->setSpacing(8);
        btnRow->addStretch(0);

        auto *openBtn = new QPushButton("Open", card);
        openBtn->setCursor(Qt::PointingHandCursor);
        openBtn->setMinimumWidth(64);
        openBtn->setMinimumHeight(20);
        openBtn->setStyleSheet(
            "QPushButton{background:#7C3E66;color:white;border:none;border-radius:6px;"
            "padding:4px 12px;font-size:10px;font-weight:bold;}"
            "QPushButton:hover{background:#93507B;}");
        connect(openBtn, &QPushButton::clicked, this, [this, roomId]() { joinRoom(roomId); });
        btnRow->addWidget(openBtn);

        if (creatorId == m_userId) {
            auto *closeBtn = new QPushButton("Close", card);
            closeBtn->setCursor(Qt::PointingHandCursor);
            closeBtn->setMinimumWidth(64);
            closeBtn->setMinimumHeight(20);
            closeBtn->setStyleSheet(
                "QPushButton{background:#E4577A;color:white;border:none;border-radius:6px;"
                "padding:4px 12px;font-size:10px;font-weight:bold;}"
                "QPushButton:hover{background:#D13D60;}");
            connect(closeBtn, &QPushButton::clicked, this, [this, roomId]() { closeRoomDirect(roomId); });
            btnRow->addWidget(closeBtn);
        }

        cardLayout->addLayout(btnRow);
        cardLayout->activate();
        card->adjustSize();

        item->setSizeHint(card->sizeHint());
        m_roomListWidget->setItemWidget(item, card);
    }
}

void StudyRoomsPage::closeRoomDirect(int roomId)
{
    QJsonObject req;
    req["action"] = "studyroom_close";
    req["userId"] = m_userId;
    req["roomId"] = roomId;
    sendRequest(req);
}

void StudyRoomsPage::onRoomItemDoubleClicked(QListWidgetItem *item)
{
    joinRoom(item->data(Qt::UserRole).toInt());
}

void StudyRoomsPage::joinRoom(int roomId)
{
    QJsonObject req;
    req["action"] = "studyroom_join";
    req["userId"] = m_userId;
    req["roomId"] = roomId;
    sendRequest(req);
}


void StudyRoomsPage::handleServerResponse(const QJsonObject &responseObj)
{
    QString action = responseObj["action"].toString();
    QString type = responseObj["type"].toString();

    if (action == "studyroom_create_response") {
        if (responseObj["status"].toString() == "success") {
            m_newRoomNameEdit->clear();
            requestRoomsForSelectedBook();
        } else {
            StyledMessageBox::error(this, "Couldn't Create Room", responseObj["message"].toString());
        }
        return;
    }

    if (action == "studyroom_list_response") {
        if (responseObj["status"].toString() == "success" &&
            responseObj["bookId"].toInt() == m_bookSelector->currentData().toInt()) {
            rebuildRoomList(responseObj["rooms"].toArray());
        }
        return;
    }

    if (action == "studyroom_join_response") {
        if (responseObj["status"].toString() == "success") {
            int roomId = responseObj["roomId"].toInt();
            bool isCreator = (responseObj["creatorId"].toInt() == m_userId);
            emit studyRoomReaderRequested(responseObj["bookId"].toInt(), roomId, isCreator,
                                          responseObj["name"].toString());
            requestRoomsForSelectedBook();
        } else {
            StyledMessageBox::error(this, "Couldn't Join Room", responseObj["message"].toString());
        }
        return;
    }

    if (action == "studyroom_leave_response" || action == "studyroom_close_response") {
        requestRoomsForSelectedBook();
        return;
    }

    if (type == "studyroom_member_joined" || type == "studyroom_member_left" || type == "studyroom_closed") {
        requestRoomsForSelectedBook();
    }
}
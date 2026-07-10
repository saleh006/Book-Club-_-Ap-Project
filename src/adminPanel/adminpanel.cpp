#include "adminpanel.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>

AdminPanel::AdminPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    switchPage(0);

    m_socket = new QTcpSocket(this);
    connect(m_socket,&QTcpSocket::readyRead,this,&AdminPanel::onReadyRead);
    m_socket->connectToHost("127.0.0.1" , 1234);

    connect(m_socket, &QTcpSocket::connected, this, [this]() {
        QJsonObject reqUsers;
        reqUsers["action"] = "get_users_list";
        m_socket->write(QJsonDocument(reqUsers).toJson(QJsonDocument::Compact) + "\n");

        QJsonObject reqBooks;
        reqBooks["action"] = "get_books_list";
        m_socket->write(QJsonDocument(reqBooks).toJson(QJsonDocument::Compact) + "\n");
    });
}

void AdminPanel::setupUi()
{
    this->resize(800, 500);
    this->setStyleSheet("background-color: #060508; color: #EAEAEA; font-family: 'Segoe UI', Arial;");

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget *sidebar = new QWidget(this);
    sidebar->setFixedWidth(220);
    sidebar->setStyleSheet("background-color: #120E14; border-left: 1px solid #1F1724;");
    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(15, 25, 15, 25);
    sidebarLayout->setSpacing(12);

    QLabel *avatarLabel = new QLabel("👤", sidebar);
    avatarLabel->setStyleSheet("font-size: 40px; border: none; background: transparent;");
    avatarLabel->setAlignment(Qt::AlignCenter);

    QLabel *nameLabel = new QLabel("Admin", sidebar);
    nameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #FFEAD2; border: none; background: transparent;");
    nameLabel->setAlignment(Qt::AlignCenter);

    QLabel *roleLabel = new QLabel("System Administrator", sidebar);
    roleLabel->setStyleSheet("font-size: 11px; color: #A594B3; border: none; background: transparent;");
    roleLabel->setAlignment(Qt::AlignCenter);

    sidebarLayout->addWidget(avatarLabel);
    sidebarLayout->addWidget(nameLabel);
    sidebarLayout->addWidget(roleLabel);
    sidebarLayout->addSpacing(15);

    m_btnMonitor = new QPushButton("📈 Server Monitor", sidebar);
    m_btnUsers = new QPushButton("👥 Manage Users", sidebar);
    m_btnBooks = new QPushButton("📚 Manage Books", sidebar);

    m_btnLogout = new QPushButton("🚪 Logout", sidebar);
    m_btnLogout->setCursor(Qt::PointingHandCursor);
    m_btnLogout->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #7C3E66; border-radius: 8px; padding: 8px; font-weight: bold; color: #D9C2D1; text-align: left; padding-left: 12px; }"
        "QPushButton:hover { background-color: rgba(124, 62, 102, 60); color: white; border: 1px solid #B06B96; }"
        );

    QString menuBtnStyle =
        "QPushButton { background-color: transparent; border: none; border-radius: 8px; padding: 10px; font-size: 13px; color: #9A8FA0; text-align: left; padding-left: 12px; }"
        "QPushButton:hover { background-color: #1F1724; color: #EAEAEA; }";

    m_btnMonitor->setStyleSheet(menuBtnStyle);
    m_btnUsers->setStyleSheet(menuBtnStyle);
    m_btnBooks->setStyleSheet(menuBtnStyle);

    m_btnMonitor->setCursor(Qt::PointingHandCursor);
    m_btnUsers->setCursor(Qt::PointingHandCursor);
    m_btnBooks->setCursor(Qt::PointingHandCursor);

    sidebarLayout->addWidget(m_btnMonitor);
    sidebarLayout->addWidget(m_btnUsers);
    sidebarLayout->addWidget(m_btnBooks);

    sidebarLayout->addStretch();
    sidebarLayout->addWidget(m_btnLogout);

    m_stackedWidget = new QStackedWidget(this);

    ServerWindow *serverPage = new ServerWindow(this);
    m_stackedWidget->addWidget(serverPage);

    QWidget * usersPage = createUsersPage();
    m_stackedWidget->addWidget(usersPage);

    QWidget *booksPage = createBooksPage();
    m_stackedWidget->addWidget(booksPage);

    mainLayout->addWidget(m_stackedWidget);
    mainLayout->addWidget(sidebar);

    connect(m_btnMonitor, &QPushButton::clicked, this, [this](){ switchPage(0); });
    connect(m_btnUsers, &QPushButton::clicked, this, [this](){ switchPage(1); });
    connect(m_btnBooks, &QPushButton::clicked, this, [this](){ switchPage(2); });
    connect(m_btnLogout, &QPushButton::clicked, this, &AdminPanel::logoutRequested);
}

QWidget* AdminPanel::createUsersPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(12);

    m_searchEdit = new QLineEdit(page);
    m_searchEdit->setPlaceholderText("🔍 Search users by username ...");
    m_searchEdit->setStyleSheet(
        "QLineEdit { background-color: #120E14; border: 1px solid #1F1724; border-radius: 6px; "
        "padding: 8px; color: #EAEAEA; font-size: 13px; }"
        "QLineEdit:focus { border: 1px solid #7C3E66; }"
        );
    layout->addWidget(m_searchEdit);

    m_usersTable = new QTableWidget(page);
    m_usersTable->setColumnCount(4);
    m_usersTable->setHorizontalHeaderLabels({"ID", "Username", "Role", "Status"});
    m_usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_usersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_usersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_usersTable->verticalHeader()->setVisible(false);

    m_usersTable->setStyleSheet(
        "QTableWidget { background-color: #09070C; border: 1px solid #1F1724; gridline-color: #1F1724; color: #EAEAEA; }"
        "QTableWidget::item:selected { background-color: #7C3E66; color: white; }"
        "QHeaderView::section { background-color: #120E14; color: #A594B3; font-weight: bold; border: 1px solid #1F1724; padding: 6px; }"
        );
    layout->addWidget(m_usersTable);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_btnBlock = new QPushButton("🚫 Block User", page);
    m_btnUnblock = new QPushButton("✅ Unblock User", page);

    m_btnBlock->setCursor(Qt::PointingHandCursor);
    m_btnUnblock->setCursor(Qt::PointingHandCursor);

    m_btnBlock->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #C0392B; border-radius: 6px; padding: 8px; font-weight: bold; color: #E6B0AA; }"
        "QPushButton:hover { background-color: rgba(192, 57, 43, 50); color: white; }"
        );
    m_btnUnblock->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #2ECC71; border-radius: 6px; padding: 8px; font-weight: bold; color: #ABEBC6; }"
        "QPushButton:hover { background-color: rgba(46, 204, 113, 50); color: white; }"
        );

    btnLayout->addWidget(m_btnBlock);
    btnLayout->addWidget(m_btnUnblock);
    layout->addLayout(btnLayout);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &AdminPanel::filterUsers);
    connect(m_btnBlock, &QPushButton::clicked, this, &AdminPanel::handleBlockUser);
    connect(m_btnUnblock, &QPushButton::clicked, this, &AdminPanel::handleUnblockUser);

    return page;
}

QWidget* AdminPanel::createBooksPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(12);

    m_bookSearchEdit = new QLineEdit(page);
    m_bookSearchEdit->setPlaceholderText("🔍 Search books by title or author...");
    m_bookSearchEdit->setStyleSheet(
        "QLineEdit { background-color: #120E14; border: 1px solid #1F1724; border-radius: 6px; "
        "padding: 8px; color: #EAEAEA; font-size: 13px; }"
        "QLineEdit:focus { border: 1px solid #7C3E66; }"
        );
    layout->addWidget(m_bookSearchEdit);

    m_booksTable = new QTableWidget(page);
    m_booksTable->setColumnCount(4);
    m_booksTable->setHorizontalHeaderLabels({"ID", "Title", "Author", "Status"});
    m_booksTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_booksTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_booksTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_booksTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_booksTable->verticalHeader()->setVisible(false);

    m_booksTable->setStyleSheet(
        "QTableWidget { background-color: #09070C; border: 1px solid #1F1724; gridline-color: #1F1724; color: #EAEAEA; }"
        "QTableWidget::item:selected { background-color: #7C3E66; color: white; }"
        "QHeaderView::section { background-color: #120E14; color: #A594B3; font-weight: bold; border: 1px solid #1F1724; padding: 6px; }"
        );
    layout->addWidget(m_booksTable);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_btnApprove = new QPushButton("✨ Approve Book", page);
    m_btnReject = new QPushButton("🗑️ Reject/Delete", page);

    m_btnApprove->setCursor(Qt::PointingHandCursor);
    m_btnReject->setCursor(Qt::PointingHandCursor);

    m_btnApprove->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #2ECC71; border-radius: 6px; padding: 8px; font-weight: bold; color: #ABEBC6; }"
        "QPushButton:hover { background-color: rgba(46, 204, 113, 50); color: white; }"
        );
    m_btnReject->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #E74C3C; border-radius: 6px; padding: 8px; font-weight: bold; color: #F5B7B1; }"
        "QPushButton:hover { background-color: rgba(231, 76, 60, 50); color: white; }"
        );

    btnLayout->addWidget(m_btnReject);
    btnLayout->addWidget(m_btnApprove);
    layout->addLayout(btnLayout);

    connect(m_bookSearchEdit,&QLineEdit::textChanged,this,&AdminPanel::filterBooks);
    connect(m_btnApprove,&QPushButton::clicked,this,&AdminPanel::handleApproveBook);
    connect(m_btnReject,&QPushButton::clicked,this,&AdminPanel::handleRejectBook);

    return page;
}

void AdminPanel::filterUsers(const QString &text)
{
    for (int i = 0; i < m_usersTable->rowCount(); ++i) {
        bool match = false;
        if (m_usersTable->item(i, 1)->text().contains(text, Qt::CaseInsensitive)) {
            match = true;
        }
        m_usersTable->setRowHidden(i, !match);
    }
}

void AdminPanel::handleBlockUser()
{
    int currentRow = m_usersTable->currentRow();
    if (currentRow < 0) return;

    QString username = m_usersTable->item(currentRow, 1)->text();
    QJsonObject packet;
    packet["action"] = "set_user_block_status";
    packet["username"] = username;
    packet["block_status"] = true;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void AdminPanel::handleUnblockUser()
{
    int currentRow = m_usersTable->currentRow();
    if (currentRow < 0) return;

    QString username = m_usersTable->item(currentRow, 1)->text();
    QJsonObject packet;
    packet["action"] = "set_user_block_status";
    packet["username"] = username;
    packet["block_status"] = false;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void AdminPanel::filterBooks(const QString &text)
{
    for (int i = 0; i < m_booksTable->rowCount(); ++i) {
        bool match = false;
        QString title = m_booksTable->item(i, 1)->text();
        QString author = m_booksTable->item(i, 2)->text();

        if (title.contains(text, Qt::CaseInsensitive) || author.contains(text, Qt::CaseInsensitive)) {
            match = true;
        }
        m_booksTable->setRowHidden(i, !match);
    }
}

void AdminPanel::handleApproveBook()
{
    int currentRow = m_booksTable->currentRow();
    if (currentRow < 0) return;

    int bookId = m_booksTable->item(currentRow, 0)->text().toInt();

    QJsonObject packet;
    packet["action"] = "set_book_active_status";
    packet["bookId"] = bookId;
    packet["active_status"] = true;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");

}

void AdminPanel::handleRejectBook()
{
    int currentRow = m_booksTable->currentRow();
    if (currentRow < 0) return;
    int bookId = m_booksTable->item(currentRow, 0)->text().toInt();

    QJsonObject packet;
    packet["action"] = "set_book_active_status";
    packet["bookId"] = bookId;
    packet["active_status"] = false;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void AdminPanel::switchPage(int index)
{
    m_stackedWidget->setCurrentIndex(index);
    updateButtonStyles(index);
}

void AdminPanel::updateButtonStyles(int currentIndex)
{
    QString normalStyle =
        "QPushButton { background-color: transparent; border: none; border-radius: 8px; padding: 12px; font-size: 14px; color: #9A8FA0; text-align: left; padding-left: 15px; }"
        "QPushButton:hover { background-color: #1F1724; color: #EAEAEA; }";

    QString activeStyle =
        "QPushButton { background-color: #7C3E66; border: none; border-radius: 8px; padding: 12px; font-size: 14px; font-weight: bold; color: #FFFFFF; text-align: left; padding-left: 15px; }";

    m_btnMonitor->setStyleSheet(currentIndex == 0 ? activeStyle : normalStyle);
    m_btnUsers->setStyleSheet(currentIndex == 1 ? activeStyle : normalStyle);
    m_btnBooks->setStyleSheet(currentIndex == 2 ? activeStyle : normalStyle);
}

void AdminPanel::mousePressEvent(QMouseEvent *event)
{
    m_usersTable->clearSelection();
    m_booksTable->clearSelection();
    QWidget::mousePressEvent(event);
}

void AdminPanel::updateRowAppearance(QTableWidget *table, int row, bool isDimmed)
{
    for (int col = 0; col < table->columnCount(); ++col) {
        QTableWidgetItem *item = table->item(row, col);
        if (item) {
            if (isDimmed) {
                item->setForeground(QColor("#6A5D75"));
                item->setBackground(QColor("#050407"));
            } else {
                item->setForeground(QColor("#EAEAEA"));
                item->setBackground(QColor("#09070C"));
            }
        }
    }
}

void AdminPanel::onReadyRead()
{
    while (m_socket->canReadLine()) {
        QByteArray line = m_socket->readLine().trimmed();
        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (doc.isNull()) continue;

        QJsonObject response = doc.object();
        QString action = response["action"].toString();

        if (action == "users_list_response" && response["status"] == "success") {
            QJsonArray users = response["data"].toArray();
            m_usersTable->setRowCount(0);

            for (int i = 0; i < users.size(); ++i) {
                QJsonObject u = users[i].toObject();
                m_usersTable->insertRow(i);

                m_usersTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
                m_usersTable->setItem(i, 1, new QTableWidgetItem(u["username"].toString()));
                m_usersTable->setItem(i, 2, new QTableWidgetItem(u["role"].toString()));

                bool isBlocked = u["isBlocked"].toBool();
                m_usersTable->setItem(i, 3, new QTableWidgetItem(isBlocked ? "Blocked" : "Active")); // وضعیت

                if (isBlocked) updateRowAppearance(m_usersTable, i, true);
            }
        }
        else if (action == "books_list_response" && response["status"] == "success") {
            QJsonArray books = response["data"].toArray();
            m_booksTable->setRowCount(0);

            for (int i = 0; i < books.size(); ++i) {
                QJsonObject b = books[i].toObject();
                m_booksTable->insertRow(i);

                m_booksTable->setItem(i, 0, new QTableWidgetItem(QString::number(b["id"].toInt())));
                m_booksTable->setItem(i, 1, new QTableWidgetItem(b["title"].toString()));
                m_booksTable->setItem(i, 2, new QTableWidgetItem(b["author"].toString()));

                bool isActive = b["isActive"].toBool();
                m_booksTable->setItem(i, 3, new QTableWidgetItem(isActive ? "Approved" : "Pending/Rejected"));

                if (!isActive) updateRowAppearance(m_booksTable, i, true);
            }
        }
        else if (action == "set_user_block_status_response" && response["status"] == "success") {
            int currentRow = m_usersTable->currentRow();
            if (currentRow >= 0) {
                bool isBlocked = response["block_status"].toBool();
                m_usersTable->item(currentRow, 3)->setText(isBlocked ? "Blocked" : "Active");
                updateRowAppearance(m_usersTable, currentRow, isBlocked);
            }
        }
        else if (action == "set_book_active_status_response" && response["status"] == "success") {
            int currentRow = m_booksTable->currentRow();
            if (currentRow >= 0) {
                bool isActive = response["active_status"].toBool();
                m_booksTable->item(currentRow, 3)->setText(isActive ? "Approved" : "Rejected");
                updateRowAppearance(m_booksTable, currentRow, !isActive);
            }
        }
    }
}
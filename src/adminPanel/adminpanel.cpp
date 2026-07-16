#include "adminpanel.h"
#include "src/adminPanel/editbookdialogadmin.h"
//#include "models.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>

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

        QJsonObject reqPublishers;
        reqPublishers["action"] = "get_publishers_list";
        m_socket->write(QJsonDocument(reqPublishers).toJson(QJsonDocument::Compact) + "\n");
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
    m_btnPublishers = new QPushButton("🧑‍💻 Manage Publishers", sidebar);

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
    m_btnPublishers->setStyleSheet(menuBtnStyle);

    m_btnMonitor->setCursor(Qt::PointingHandCursor);
    m_btnUsers->setCursor(Qt::PointingHandCursor);
    m_btnBooks->setCursor(Qt::PointingHandCursor);
    m_btnPublishers->setCursor(Qt::PointingHandCursor);

    sidebarLayout->addWidget(m_btnMonitor);
    sidebarLayout->addWidget(m_btnUsers);
    sidebarLayout->addWidget(m_btnPublishers);
    sidebarLayout->addWidget(m_btnBooks);

    sidebarLayout->addStretch();
    sidebarLayout->addWidget(m_btnLogout);

    m_stackedWidget = new QStackedWidget(this);

    ServerWindow *serverPage = new ServerWindow(this);
    m_stackedWidget->addWidget(serverPage);

    QWidget * usersPage = createUsersPage();
    m_stackedWidget->addWidget(usersPage);

    QWidget *publishersPage = createPublishersPage();
    m_stackedWidget->addWidget(publishersPage);

    QWidget *booksPage = createBooksPage();
    m_stackedWidget->addWidget(booksPage);

    mainLayout->addWidget(m_stackedWidget);
    mainLayout->addWidget(sidebar);

    connect(m_btnMonitor, &QPushButton::clicked, this, [this](){ switchPage(0); });
    connect(m_btnUsers, &QPushButton::clicked, this, [this](){ switchPage(1); });
    connect(m_btnPublishers, &QPushButton::clicked, this, [this](){ switchPage(2); });
    connect(m_btnBooks, &QPushButton::clicked, this, [this](){ switchPage(3); });
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
    m_usersTable->setColumnCount(5);
    m_usersTable->setHorizontalHeaderLabels({"ID", "Username", "Role", "Status" , "Register Date"});
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
    m_btnUserDetails = new QPushButton("👁 View Details", page);
    m_btnDeleteUser = new QPushButton("🗑️ Delete User", page);

    m_btnBlock->setCursor(Qt::PointingHandCursor);
    m_btnUnblock->setCursor(Qt::PointingHandCursor);
    m_btnUserDetails->setCursor(Qt::PointingHandCursor);
    m_btnDeleteUser->setCursor(Qt::PointingHandCursor);

    m_btnBlock->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #C0392B; border-radius: 6px; padding: 8px; font-weight: bold; color: #E6B0AA; }"
        "QPushButton:hover { background-color: rgba(192, 57, 43, 50); color: white; }"
        );
    m_btnUnblock->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #2ECC71; border-radius: 6px; padding: 8px; font-weight: bold; color: #ABEBC6; }"
        "QPushButton:hover { background-color: rgba(46, 204, 113, 50); color: white; }"
        );
    m_btnUserDetails->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #3498DB; border-radius: 6px; padding: 8px; font-weight: bold; color: #AED6F1; }"
        "QPushButton:hover { background-color: rgba(52, 152, 219, 50); color: white; }"
        );
    m_btnDeleteUser->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #C0392B; border-radius: 6px; padding: 8px; font-weight: bold; color: #E6B0AA; }"
        "QPushButton:hover { background-color: rgba(192, 57, 43, 50); color: white; }"
        );

    btnLayout->addWidget(m_btnBlock);
    btnLayout->addWidget(m_btnUnblock);
    btnLayout->addWidget(m_btnDeleteUser);
    btnLayout->addWidget(m_btnUserDetails);
    layout->addLayout(btnLayout);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &AdminPanel::filterUsers);
    connect(m_btnBlock, &QPushButton::clicked, this, &AdminPanel::handleBlockUser);
    connect(m_btnUnblock, &QPushButton::clicked, this, &AdminPanel::handleUnblockUser);
    connect(m_btnUserDetails, &QPushButton::clicked, this, &AdminPanel::handleViewUserDetails);
    connect(m_btnDeleteUser, &QPushButton::clicked, this, &AdminPanel::handleDeleteUser);

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
    m_btnReject = new QPushButton("❌ Reject", page);
    m_btnDeleteBook = new QPushButton("🗑️ Delete Book", page);
    m_btnBookDetails = new QPushButton("👁 View Details", page);
    m_btnEditBook = new QPushButton("✏️ Edit Book", page);

    m_btnApprove->setCursor(Qt::PointingHandCursor);
    m_btnReject->setCursor(Qt::PointingHandCursor);
    m_btnDeleteBook->setCursor(Qt::PointingHandCursor);
    m_btnBookDetails->setCursor(Qt::PointingHandCursor);
    m_btnEditBook->setCursor(Qt::PointingHandCursor);

    m_btnApprove->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #2ECC71; border-radius: 6px; padding: 8px; font-weight: bold; color: #ABEBC6; }"
        "QPushButton:hover { background-color: rgba(46, 204, 113, 50); color: white; }"
        );
    m_btnReject->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #E74C3C; border-radius: 6px; padding: 8px; font-weight: bold; color: #F5B7B1; }"
        "QPushButton:hover { background-color: rgba(231, 76, 60, 50); color: white; }"
        );
    m_btnDeleteBook->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #C0392B; border-radius: 6px; padding: 8px; font-weight: bold; color: #E6B0AA; }"
        "QPushButton:hover { background-color: rgba(192, 57, 43, 50); color: white; }"
        );
    m_btnBookDetails->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #3498DB; border-radius: 6px; padding: 8px; font-weight: bold; color: #AED6F1; }"
        "QPushButton:hover { background-color: rgba(52, 152, 219, 50); color: white; }"
        );
    m_btnEditBook->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #E9A23B; border-radius: 6px; padding: 8px; font-weight: bold; color: #F5D7A0; }"
        "QPushButton:hover { background-color: rgba(233, 162, 59, 50); color: white; }"
        );

    btnLayout->addWidget(m_btnBookDetails);
    btnLayout->addWidget(m_btnEditBook);
    btnLayout->addWidget(m_btnDeleteBook);
    btnLayout->addWidget(m_btnReject);
    btnLayout->addWidget(m_btnApprove);
    layout->addLayout(btnLayout);

    connect(m_bookSearchEdit,&QLineEdit::textChanged,this,&AdminPanel::filterBooks);
    connect(m_btnApprove,&QPushButton::clicked,this,&AdminPanel::handleApproveBook);
    connect(m_btnReject,&QPushButton::clicked,this,&AdminPanel::handleRejectBook);
    connect(m_btnDeleteBook, &QPushButton::clicked, this, &AdminPanel::handleDeleteBook);
    connect(m_btnBookDetails, &QPushButton::clicked, this, &AdminPanel::handleViewBookDetails);
    connect(m_btnEditBook, &QPushButton::clicked, this, &AdminPanel::handleEditBook);

    return page;
}

QWidget* AdminPanel::createPublishersPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(12);

    m_publisherSearchEdit = new QLineEdit(page);
    m_publisherSearchEdit->setPlaceholderText("🔍 Search publishers by username or name ...");
    m_publisherSearchEdit->setStyleSheet(
        "QLineEdit { background-color: #120E14; border: 1px solid #1F1724; border-radius: 6px; "
        "padding: 8px; color: #EAEAEA; font-size: 13px; }"
        "QLineEdit:focus { border: 1px solid #7C3E66; }"
        );
    layout->addWidget(m_publisherSearchEdit);

    m_publishersTable = new QTableWidget(page);
    m_publishersTable->setColumnCount(6);
    m_publishersTable->setHorizontalHeaderLabels({"ID", "Username", "Publisher Name", "Books", "Status", "Register Date"});
    m_publishersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_publishersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_publishersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_publishersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_publishersTable->verticalHeader()->setVisible(false);
    m_publishersTable->setStyleSheet(
        "QTableWidget { background-color: #09070C; border: 1px solid #1F1724; gridline-color: #1F1724; color: #EAEAEA; }"
        "QTableWidget::item:selected { background-color: #7C3E66; color: white; }"
        "QHeaderView::section { background-color: #120E14; color: #A594B3; font-weight: bold; border: 1px solid #1F1724; padding: 6px; }"
        );
    layout->addWidget(m_publishersTable);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_btnBlockPublisher = new QPushButton("🚫 Block Publisher", page);
    m_btnUnblockPublisher = new QPushButton("✅ Unblock Publisher", page);
    m_btnPublisherDetails = new QPushButton("👁 View Details", page);
    m_btnDeletePublisher = new QPushButton("🗑️ Delete Publisher", page);

    m_btnBlockPublisher->setCursor(Qt::PointingHandCursor);
    m_btnUnblockPublisher->setCursor(Qt::PointingHandCursor);
    m_btnPublisherDetails->setCursor(Qt::PointingHandCursor);
    m_btnDeletePublisher->setCursor(Qt::PointingHandCursor);

    m_btnBlockPublisher->setStyleSheet(m_btnBlock->styleSheet());
    m_btnUnblockPublisher->setStyleSheet(m_btnUnblock->styleSheet());
    m_btnPublisherDetails->setStyleSheet(m_btnUserDetails->styleSheet());
    m_btnDeletePublisher->setStyleSheet(m_btnDeleteUser->styleSheet());

    btnLayout->addWidget(m_btnBlockPublisher);
    btnLayout->addWidget(m_btnUnblockPublisher);
    btnLayout->addWidget(m_btnDeletePublisher);
    btnLayout->addWidget(m_btnPublisherDetails);
    layout->addLayout(btnLayout);

    connect(m_publisherSearchEdit, &QLineEdit::textChanged, this, &AdminPanel::filterPublishers);
    connect(m_btnBlockPublisher, &QPushButton::clicked, this, &AdminPanel::handleBlockPublisher);
    connect(m_btnUnblockPublisher, &QPushButton::clicked, this, &AdminPanel::handleUnblockPublisher);
    connect(m_btnPublisherDetails, &QPushButton::clicked, this, &AdminPanel::handleViewPublisherDetails);
    connect(m_btnDeletePublisher, &QPushButton::clicked, this, &AdminPanel::handleDeletePublisher);

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

void AdminPanel::handleViewUserDetails()
{
    int currentRow = m_usersTable->currentRow();
    if (currentRow < 0) return;

    QString username = m_usersTable->item(currentRow, 1)->text();

    QJsonObject packet;
    packet["action"] = "get_user_details";
    packet["username"] = username;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void AdminPanel::handleDeleteUser()
{
    int currentRow = m_usersTable->currentRow();
    if (currentRow < 0) return;

    QString username = m_usersTable->item(currentRow, 1)->text();
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Delete",
                                                              "Are you sure you want to completely delete user: " + username + "?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QJsonObject packet;
        packet["action"] = "delete_account";
        packet["username"] = username;
        m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
    }
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

void AdminPanel::handleDeleteBook()
{
    int currentRow = m_booksTable->currentRow();
    if (currentRow < 0) return;
    int bookId = m_booksTable->item(currentRow, 0)->text().toInt();

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Delete",
                                                              "Are you sure you want to completely delete this book from the database?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QJsonObject packet;
        packet["action"] = "books_delete";
        packet["bookId"] = bookId;
        m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
    }
}

void AdminPanel::handleViewBookDetails()
{
    int currentRow = m_booksTable->currentRow();
    if (currentRow < 0) return;
    int bookId = m_booksTable->item(currentRow, 0)->text().toInt();

    m_pendingBookDetailPurpose = "view";
    QJsonObject packet;
    packet["action"] = "get_book_details";
    packet["bookId"] = bookId;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void AdminPanel::handleEditBook()
{
    int currentRow = m_booksTable->currentRow();
    if (currentRow < 0) return;
    int bookId = m_booksTable->item(currentRow, 0)->text().toInt();

    m_pendingBookDetailPurpose = "edit";
    QJsonObject packet;
    packet["action"] = "get_book_details";
    packet["bookId"] = bookId;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void AdminPanel::filterPublishers(const QString &text)
{
    for (int i = 0; i < m_publishersTable->rowCount(); ++i) {
        bool match = false;
        if (m_publishersTable->item(i, 1)->text().contains(text, Qt::CaseInsensitive) ||
            m_publishersTable->item(i, 2)->text().contains(text, Qt::CaseInsensitive)) {
            match = true;
        }
        m_publishersTable->setRowHidden(i, !match);
    }
}

void AdminPanel::handleBlockPublisher()
{
    int currentRow = m_publishersTable->currentRow();
    if (currentRow < 0) return;

    QString username = m_publishersTable->item(currentRow, 1)->text();
    QJsonObject packet;
    packet["action"] = "set_user_block_status";
    packet["username"] = username;
    packet["block_status"] = true;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void AdminPanel::handleUnblockPublisher()
{
    int currentRow = m_publishersTable->currentRow();
    if (currentRow < 0) return;

    QString username = m_publishersTable->item(currentRow, 1)->text();
    QJsonObject packet;
    packet["action"] = "set_user_block_status";
    packet["username"] = username;
    packet["block_status"] = false;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void AdminPanel::handleViewPublisherDetails()
{
    int currentRow = m_publishersTable->currentRow();
    if (currentRow < 0) return;

    QString username = m_publishersTable->item(currentRow, 1)->text();

    QJsonObject packet;
    packet["action"] = "get_publisher_details";
    packet["username"] = username;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void AdminPanel::handleDeletePublisher()
{
    int currentRow = m_publishersTable->currentRow();
    if (currentRow < 0) return;

    QString username = m_publishersTable->item(currentRow, 1)->text();
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Delete",
                                                              "Are you sure you want to completely delete publisher: " + username + "?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QJsonObject packet;
        packet["action"] = "delete_account";
        packet["username"] = username;
        m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
    }
}

void AdminPanel::refreshUsersTable() {
    QJsonObject request;
    request["action"] = "get_users_list";
    m_socket->write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");
    m_socket->flush();
}

void AdminPanel::refreshBooksTable() {
    QJsonObject request;
    request["action"] = "get_books_list";
    m_socket->write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");
    m_socket->flush();
}

void AdminPanel::refreshPublishersTable() {
    QJsonObject request;
    request["action"] = "get_publishers_list";
    m_socket->write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");
    m_socket->flush();
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
    m_btnPublishers->setStyleSheet(currentIndex == 2 ? activeStyle : normalStyle);
    m_btnBooks->setStyleSheet(currentIndex == 3 ? activeStyle : normalStyle);
}

void AdminPanel::mousePressEvent(QMouseEvent *event)
{
    m_usersTable->clearSelection();
    m_booksTable->clearSelection();
    m_publishersTable->clearSelection();
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

        if (response.contains("type") && response["type"].toString() == "table_refresh_required") {
            QString targetTable = response["target_table"].toString();
            if (targetTable == "book") {
                refreshBooksTable();
            } else if (targetTable == "users") {
                refreshUsersTable();
            }
            continue;
        }

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
                m_usersTable->setItem(i, 3, new QTableWidgetItem(isBlocked ? "Blocked" : "Active"));
                m_usersTable->setItem(i, 4, new QTableWidgetItem(u["registerDate"].toString()));

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
        else if (action == "publishers_list_response" && response["status"] == "success") {
            QJsonArray publishers = response["data"].toArray();
            m_publishersTable->setRowCount(0);

            for (int i = 0; i < publishers.size(); ++i) {
                QJsonObject p = publishers[i].toObject();
                m_publishersTable->insertRow(i);

                m_publishersTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
                m_publishersTable->setItem(i, 1, new QTableWidgetItem(p["username"].toString()));
                m_publishersTable->setItem(i, 2, new QTableWidgetItem(p["fullName"].toString()));
                m_publishersTable->setItem(i, 3, new QTableWidgetItem(QString::number(p["publishedBooksCount"].toInt())));
                bool isBlocked = p["isBlocked"].toBool();
                m_publishersTable->setItem(i, 4, new QTableWidgetItem(isBlocked ? "Blocked" : "Active"));
                m_publishersTable->setItem(i, 5, new QTableWidgetItem(p["registerDate"].toString()));

                if (isBlocked) updateRowAppearance(m_publishersTable, i, true);
            }
        }
        else if (action == "set_user_block_status_response" && response["status"] == "success") {
            bool isBlocked = response["block_status"].toBool();
            QString targetUser = response["username"].toString();

            for(int i=0; i<m_usersTable->rowCount(); i++) {
                if(m_usersTable->item(i, 1)->text() == targetUser) {
                    m_usersTable->item(i, 3)->setText(isBlocked ? "Blocked" : "Active");
                    updateRowAppearance(m_usersTable, i, isBlocked);
                    break;
                }
            }
            for(int i=0; i<m_publishersTable->rowCount(); i++) {
                if(m_publishersTable->item(i, 1)->text() == targetUser) {
                    m_publishersTable->item(i, 4)->setText(isBlocked ? "Blocked" : "Active");
                    updateRowAppearance(m_publishersTable, i, isBlocked);
                    break;
                }
            }
        }
        else if (action == "delete_book_response" && response["status"] == "success") {
            int currentRow = m_booksTable->currentRow();
            if (currentRow >= 0) {
                m_booksTable->removeRow(currentRow);
            }
            QMessageBox::information(this, "Success", "Book completely deleted from the database.");
        }
        else if (action == "user_details_response" && response["status"] == "success") {
            showUserDetailsDialog(response["data"].toObject());
        }
        else if (action == "publisher_details_response" && response["status"] == "success") {
            showPublisherDetailsDialog(response["data"].toObject());
        }
        else if (action == "delete_account_response" && response["status"] == "success") {
            QMessageBox::information(this, "Success", "Account deleted successfully from the database.");
            refreshUsersTable();
            refreshPublishersTable();
        }
        else if (action == "book_details_response" && response["status"].toString() == "success") {
            QJsonObject data = response["data"].toObject();
            if (m_pendingBookDetailPurpose == "edit") {
                Book b;
                b.id = data["id"].toInt();
                b.title = data["title"].toString();
                b.author = data["author"].toString();
                b.genre = data["genre"].toString();
                b.description = data["description"].toString();
                b.price = data["price"].toDouble();
                b.coverImagePath = data["coverImagePath"].toString();
                b.pdfPath = data["pdfPath"].toString();
                b.averageRating = data["averageRating"].toDouble();
                b.status = data["isActive"].toBool();

                AddEditBookDialog dialog(b, this);
                if (dialog.exec() == QDialog::Accepted) {
                    Book updated = dialog.resultBook();
                    QJsonObject packet;
                    packet["action"] = "book_update";
                    packet["id"] = updated.id;
                    packet["title"] = updated.title;
                    packet["author"] = updated.author;
                    packet["genre"] = updated.genre;
                    packet["description"] = updated.description;
                    packet["price"] = updated.price;
                    packet["coverImagePath"] = updated.coverImagePath;
                    packet["pdfPath"] = updated.pdfPath;
                    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
                }
            } else {
                showBookDetailsDialog(data);
            }
        }
        else if (action == "book_update_response") {
            if (response["status"].toString() == "success") {
                QMessageBox::information(this, "Success", "Book information updated successfully.");
                refreshBooksTable();
            } else {
                QMessageBox::warning(this, "Update Failed", response["message"].toString());
            }
        }
    }
}
void AdminPanel::showUserDetailsDialog(const QJsonObject &data)
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("User Profile Details");
    dialog->resize(620,680);
    dialog->setStyleSheet("background-color: #09070C; color: #EAEAEA; font-family: 'Segoe UI', Arial;");

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    mainLayout->setContentsMargins(24,24,24,20);
    mainLayout->setSpacing(14);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(16);

    QLabel *avatar = new QLabel("👤", dialog);
    avatar->setFixedSize(64,64);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet("font-size: 30px; background-color: #1F1724; border-radius: 32px; color: #DDA0DD;");

    QVBoxLayout *headerTextLayout = new QVBoxLayout();
    headerTextLayout->setSpacing(2);

    QLabel *nameLabel = new QLabel(data["fullName"].toString(), dialog);
    nameLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #FFFFFF;");

    QLabel *usernameLabel = new QLabel("@" + data["username"].toString(), dialog);
    usernameLabel->setStyleSheet("font-size: 12px; color: #9A8FA0;");

    QHBoxLayout *badgeRow = new QHBoxLayout();
    badgeRow->setSpacing(8);

    QLabel *roleBadge = new QLabel(data["role"].toString().toUpper(), dialog);
    roleBadge->setStyleSheet(
        "background-color: #5C2E4C; color: #FFEAD2; font-size: 10px; font-weight: bold; "
        "padding: 3px 10px; border-radius: 8px; letter-spacing: 1px;");
    roleBadge->setFixedHeight(20);

    bool isBlocked = data["isBlocked"].toBool();
    QLabel *statusBadge = new QLabel(isBlocked ? "🚫 BLOCKED" : "✅ ACTIVE", dialog);
    statusBadge->setStyleSheet(
        QString("background-color: %1; color: white; font-size: 10px; font-weight: bold; "
                "padding: 3px 10px; border-radius: 8px; letter-spacing: 1px;")
            .arg(isBlocked ? "#8d1f1f" : "#268730"));
    statusBadge->setFixedHeight(20);

    badgeRow->addWidget(roleBadge);
    badgeRow->addWidget(statusBadge);
    badgeRow->addStretch();

    headerTextLayout->addWidget(nameLabel);
    headerTextLayout->addWidget(usernameLabel);
    headerTextLayout->addSpacing(4);
    headerTextLayout->addLayout(badgeRow);

    headerLayout->addWidget(avatar);
    headerLayout->addLayout(headerTextLayout , 1);
    mainLayout->addLayout(headerLayout);

    QString cardStyle =
        "QGroupBox { border: 1px solid #1F1724; border-radius: 10px; margin-top: 8px; padding-top: 16px; "
        "color: #A594B3; font-weight: bold; background-color: #0F0C12; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }";

    QGroupBox *infoBox = new QGroupBox("Account Information", dialog);
    infoBox->setStyleSheet(cardStyle);
    QFormLayout *formLayout = new QFormLayout(infoBox);
    formLayout->setLabelAlignment(Qt::AlignRight);
    formLayout->setHorizontalSpacing(16);
    formLayout->setVerticalSpacing(8);

    QString labelStyle = "color: #9A8FA0; font-size: 12px; border: none; background: transparent;";
    QString valueStyle = "color: #EAEAEA; font-size: 13px; font-weight: 600; border: none; background: transparent;";

    auto addRow = [&](const QString &title, const QString &value) {
        QLabel *lbl = new QLabel(title);
        lbl->setStyleSheet(labelStyle);
        QLabel *val = new QLabel(value.isEmpty() ? "_" : value);
        val->setStyleSheet(valueStyle);
        formLayout->addRow(lbl, val);
    };

    addRow("Email:", data["email"].toString());
    addRow("Registered:", data["registerDate"].toString());
    addRow("Card Items:",QString::number(data["cartItemCount"].toInt()));
    addRow("Cart Total:", "$" + QString::number(data["cartTotal"].toDouble(), 'f', 2));

    mainLayout->addWidget(infoBox);

    QGroupBox *statsBox = new QGroupBox("Library Activity", dialog);
    statsBox->setStyleSheet(cardStyle);
    QHBoxLayout *statsLayout = new QHBoxLayout(statsBox);

    auto createStatWidget = [&](const QString &title, const QString &valueText, const QString &color) {
        QWidget *w = new QWidget();
        w->setStyleSheet("background-color: #120E14; border-radius: 8px;");
        QVBoxLayout *vl = new QVBoxLayout(w);
        vl->setContentsMargins(8,10,8,10);
        QLabel *c = new QLabel(valueText);
        c->setAlignment(Qt::AlignCenter);
        c->setStyleSheet(QString("font-size: 22px; color: %1; font-weight: bold; borde: none; backgroud: transparent").arg(color));
        QLabel *t = new QLabel(title);
        t->setAlignment(Qt::AlignCenter);
        t->setStyleSheet("font-size: 11px; color: #9A8FA0; border: none; background: transparent;");
        vl->addWidget(c);
        vl->addWidget(t);
        return w;
    };

    statsLayout->addWidget(createStatWidget("Owned Books", QString::number(data["ownedBooksCount"].toInt()), "#7C3E66"));
    statsLayout->addWidget(createStatWidget("Wishlist", QString::number(data["wishlistCount"].toInt()), "#DDA0DD"));
    statsLayout->addWidget(createStatWidget("Purchases", QString::number(data["totalPurchases"].toInt()), "#3498DB"));
    statsLayout->addWidget(createStatWidget("Total Spent", "$" + QString::number(data["totalSpent"].toDouble(), 'f', 2), "#2ECC71"));

    mainLayout->addWidget(statsBox);

    QTabWidget *tabs = new QTabWidget(dialog);
    tabs->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #1F1724; border-radius: 8px; background-color: #09070C; top: -1px; }"
        "QTabBar::tab { background-color: #120E14; color: #9A8FA0; padding: 8px 16px; border: 1px solid #1F1724; "
        "border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; font-size: 12px; }"
        "QTabBar::tab:selected { background-color: #1F1724; color: #EAEAEA; font-weight: bold; }"
        "QTabBar::tab:hover { color: #EAEAEA; }");

    QString tableStyle =
        "QTableWidget { background-color: #09070C; border: none; gridline-color: #1F1724; color: #EAEAEA; }"
        "QTableWidget::item { padding: 4px; }"
        "QTableWidget::item:selected { background-color: #7C3E66; color: white; }"
        "QHeaderView::section { background-color: #120E14; color: #A594B3; font-weight: bold; border: 1px solid #1F1724; padding: 6px; }";

    auto makeEmptyRow = [](QTableWidget *table, int columnSpan, const QString &message) {
        table->setRowCount(1);
        table->setSpan(0, 0, 1, columnSpan);
        QTableWidgetItem *empty = new QTableWidgetItem(message);
        empty->setTextAlignment(Qt::AlignCenter);
        empty->setForeground(QColor("#6B6470"));
        table->setItem(0, 0, empty);
    };

    QJsonArray ownedArr = data["ownedBooks"].toArray();
    QTableWidget *ownedTable = new QTableWidget(ownedArr.size(), 2, dialog);
    ownedTable->setHorizontalHeaderLabels({"Title", "Author"});
    ownedTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ownedTable->verticalHeader()->setVisible(false);
    ownedTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ownedTable->setSelectionMode(QAbstractItemView::NoSelection);
    ownedTable->setStyleSheet(tableStyle);
    for (int i = 0; i < ownedArr.size(); ++i) {
        QJsonObject b = ownedArr[i].toObject();
        ownedTable->setItem(i, 0, new QTableWidgetItem(b["title"].toString()));
        ownedTable->setItem(i, 1, new QTableWidgetItem(b["author"].toString()));
    }
    if (ownedArr.isEmpty()) makeEmptyRow(ownedTable, 2, "No books owned yet.");
    tabs->addTab(ownedTable, QString("📚 Owned (%1)").arg(data["ownedBooksCount"].toInt()));

    QJsonArray wishlistArr = data["wishlist"].toArray();
    QTableWidget *wishTable = new QTableWidget(wishlistArr.size(), 2, dialog);
    wishTable->setHorizontalHeaderLabels({"Title", "Author"});
    wishTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    wishTable->verticalHeader()->setVisible(false);
    wishTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    wishTable->setSelectionMode(QAbstractItemView::NoSelection);
    wishTable->setStyleSheet(tableStyle);
    for (int i = 0; i < wishlistArr.size(); ++i) {
        QJsonObject b = wishlistArr[i].toObject();
        wishTable->setItem(i, 0, new QTableWidgetItem(b["title"].toString()));
        wishTable->setItem(i, 1, new QTableWidgetItem(b["author"].toString()));
    }
    if (wishlistArr.isEmpty()) makeEmptyRow(wishTable, 2, "Wishlist is empty.");
    tabs->addTab(wishTable, QString("💜 Wishlist (%1)").arg(data["wishlistCount"].toInt()));

    QJsonArray purchasesArr = data["purchaseHistory"].toArray();
    QTableWidget *purchTable = new QTableWidget(purchasesArr.size(), 3, dialog);
    purchTable->setHorizontalHeaderLabels({"Date", "Items", "Total"});
    purchTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    purchTable->verticalHeader()->setVisible(false);
    purchTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    purchTable->setSelectionMode(QAbstractItemView::NoSelection);
    purchTable->setStyleSheet(tableStyle);
    for (int i = 0; i < purchasesArr.size(); ++i) {
        QJsonObject p = purchasesArr[i].toObject();
        purchTable->setItem(i, 0, new QTableWidgetItem(p["date"].toString()));
        purchTable->setItem(i, 1, new QTableWidgetItem(QString::number(p["itemCount"].toInt())));
        purchTable->setItem(i, 2, new QTableWidgetItem("$" + QString::number(p["total"].toDouble(), 'f', 2)));
    }
    if (purchasesArr.isEmpty()) makeEmptyRow(purchTable, 3, "No purchases yet.");
    tabs->addTab(purchTable, QString("🧾 Purchases (%1)").arg(data["totalPurchases"].toInt()));

    mainLayout->addWidget(tabs, 1);


    QPushButton *closeBtn = new QPushButton("Close", dialog);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #1F1724; border: none; border-radius: 6px; padding: 10px; color: white; font-weight: bold; }"
        "QPushButton:hover { background-color: #7C3E66; }"
        );
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);

    QHBoxLayout *footerLayout = new QHBoxLayout();
    footerLayout->addStretch();
    footerLayout->addWidget(closeBtn);
    mainLayout->addLayout(footerLayout);

    dialog->exec();
    dialog->deleteLater();
}

void AdminPanel::showPublisherDetailsDialog(const QJsonObject &data)
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Publisher Profile Details");
    dialog->resize(640,680);
    dialog->setStyleSheet("background-color: #09070C; color: #EAEAEA; font-family: 'Segoe UI', Arial;");

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    mainLayout->setContentsMargins(24,24,24,20);
    mainLayout->setSpacing(14);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(16);

    QLabel *avatar = new QLabel("🧑‍💻", dialog);
    avatar->setFixedSize(64,64);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet("font-size: 28px; background-color: #1F1724; border-radius: 32px;");

    QVBoxLayout *headerTextLayout = new QVBoxLayout();
    headerTextLayout->setSpacing(2);

    QLabel *nameLabel = new QLabel(data["fullName"].toString(), dialog);
    nameLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #FFFFFF;");

    QLabel *usernameLabel = new QLabel("@" + data["username"].toString(), dialog);
    usernameLabel->setStyleSheet("font-size: 12px; color: #9A8FA0;");

    QHBoxLayout *badgeRow = new QHBoxLayout();
    badgeRow->setSpacing(8);

    QLabel *roleBadge = new QLabel("PUBLISHER PARTNER", dialog);
    roleBadge->setStyleSheet(
        "background-color: rgba(124, 62, 102, 90); color: #FFEAD2; font-size: 10px; font-weight: bold; "
        "padding: 3px 10px; border-radius: 8px; letter-spacing: 1px;");
    roleBadge->setFixedHeight(20);

    bool isBlocked = data["isBlocked"].toBool();
    QLabel *statusBadge = new QLabel(isBlocked ? "🚫 SUSPENDED" : "✅ ACTIVE", dialog);
    statusBadge->setStyleSheet(
        QString("background-color: %1; color: white; font-size: 10px; font-weight: bold; "
                "padding: 3px 10px; border-radius: 8px; letter-spacing: 1px;")
            .arg(isBlocked ? "rgba(192,57,43,180)" : "rgba(46,204,113,150)"));
    statusBadge->setFixedHeight(20);

    badgeRow->addWidget(roleBadge);
    badgeRow->addWidget(statusBadge);
    badgeRow->addStretch();

    headerTextLayout->addWidget(nameLabel);
    headerTextLayout->addWidget(usernameLabel);
    headerTextLayout->addSpacing(4);
    headerTextLayout->addLayout(badgeRow);

    headerLayout->addWidget(avatar);
    headerLayout->addLayout(headerTextLayout, 1);
    mainLayout->addLayout(headerLayout);

    QString cardStyle =
        "QGroupBox { border: 1px solid #1F1724; border-radius: 10px; margin-top: 8px; padding-top: 16px; "
        "color: #A594B3; font-weight: bold; background-color: #0F0C12; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }";

    QGroupBox *infoBox = new QGroupBox("Account Information", dialog);
    infoBox->setStyleSheet(cardStyle);
    QFormLayout *formLayout = new QFormLayout(infoBox);
    formLayout->setLabelAlignment(Qt::AlignRight);
    formLayout->setHorizontalSpacing(16);
    formLayout->setVerticalSpacing(8);

    QString labelStyle = "color: #9A8FA0; font-size: 13px;";
    QString valueStyle = "color: #EAEAEA; font-size: 14px; font-weight: bold;";


    auto addRow = [&](const QString &title, const QString &value) {
        QLabel *lbl = new QLabel(title);
        lbl->setStyleSheet(labelStyle);
        QLabel *val = new QLabel(value.isEmpty() ? "_" : value);
        val->setStyleSheet(valueStyle);
        formLayout->addRow(lbl, val);
    };

    addRow("Email:", data["email"].toString());
    addRow("Registered:", data["registerDate"].toString());

    mainLayout->addWidget(infoBox);

    QGroupBox *statsBox = new QGroupBox("Performance Dashboard", dialog);
    statsBox->setStyleSheet(cardStyle);
    QGridLayout *statsLayout = new QGridLayout(statsBox);
    statsLayout->setHorizontalSpacing(10);
    statsLayout->setVerticalSpacing(10);

    auto createStatWidget = [&](const QString &title, const QString &valueText, const QString &color) {
        QWidget *w = new QWidget();
        w->setStyleSheet("background-color: #120E14; border-radius: 8px;");
        QVBoxLayout *vl = new QVBoxLayout(w);
        vl->setContentsMargins(8, 10, 8, 10);
        QLabel *c = new QLabel(valueText);
        c->setAlignment(Qt::AlignCenter);
        c->setStyleSheet(QString("font-size: 20px; color: %1; font-weight: bold; border: none; background: transparent;").arg(color));
        QLabel *t = new QLabel(title);
        t->setAlignment(Qt::AlignCenter);
        t->setStyleSheet("font-size: 10px; color: #9A8FA0; border: none; background: transparent;");
        vl->addWidget(c);
        vl->addWidget(t);
        return w;
    };

    QString booksCount = QString::number(data["publishedBooksCount"].toInt());
    QString salesCount = QString::number(data["totalSales"].toInt());

    double avgRating = data["averageRating"].toDouble();
    QString ratingStr = avgRating > 0.0 ? QString::number(avgRating, 'f', 1) + " ⭐" : "N/A";

    double income = data["totalIncome"].toDouble();
    QString incomeStr = "$" + QString::number(income, 'f', 2);

    statsLayout->addWidget(createStatWidget("Total Books", booksCount, "#DDA0DD"), 0, 0);
    statsLayout->addWidget(createStatWidget("Total Sales", salesCount, "#3498DB"), 0, 1);
    statsLayout->addWidget(createStatWidget("Avg Rating", ratingStr, "#F1C40F"), 0, 2);
    statsLayout->addWidget(createStatWidget("Est. Income", incomeStr, "#2ECC71"), 0, 3);

    mainLayout->addWidget(statsBox);

    QGroupBox *booksBox = new QGroupBox("Published Catalog", dialog);
    booksBox->setStyleSheet(cardStyle);
    QVBoxLayout *booksBoxLayout = new QVBoxLayout(booksBox);

    QJsonArray booksArr = data["publishedBooks"].toArray();
    QTableWidget *booksTable = new QTableWidget(booksArr.size(), 5, dialog);
    booksTable->setHorizontalHeaderLabels({"Title", "Genre", "Price", "Sales", "Rating"});
    booksTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    booksTable->verticalHeader()->setVisible(false);
    booksTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    booksTable->setSelectionMode(QAbstractItemView::NoSelection);
    booksTable->setStyleSheet(
        "QTableWidget { background-color: #09070C; border: 1px solid #1F1724; gridline-color: #1F1724; color: #EAEAEA; }"
        "QTableWidget::item { padding: 4px; }"
        "QHeaderView::section { background-color: #120E14; color: #A594B3; font-weight: bold; border: 1px solid #1F1724; padding: 6px; }");

    for (int i = 0; i < booksArr.size(); ++i) {
        QJsonObject b = booksArr[i].toObject();
        bool active = b["isActive"].toBool();

        QTableWidgetItem *titleItem = new QTableWidgetItem(b["title"].toString());
        if (!active) titleItem->setForeground(QColor("#6B6470"));
        booksTable->setItem(i, 0, titleItem);

        booksTable->setItem(i, 1, new QTableWidgetItem(b["genre"].toString()));
        booksTable->setItem(i, 2, new QTableWidgetItem("$" + QString::number(b["price"].toDouble(), 'f', 2)));
        booksTable->setItem(i, 3, new QTableWidgetItem(QString::number(b["totalSales"].toInt())));

        double r = b["averageRating"].toDouble();
        booksTable->setItem(i, 4, new QTableWidgetItem(r > 0.0 ? QString::number(r, 'f', 1) + " ⭐" : "—"));
    }
    if (booksArr.isEmpty()) {
        booksTable->setRowCount(1);
        booksTable->setSpan(0, 0, 1, 5);
        QTableWidgetItem *empty = new QTableWidgetItem("No books published yet.");
        empty->setTextAlignment(Qt::AlignCenter);
        empty->setForeground(QColor("#6B6470"));
        booksTable->setItem(0, 0, empty);
    }

    booksBoxLayout->addWidget(booksTable);
    mainLayout->addWidget(booksBox, 1);

    QPushButton *closeBtn = new QPushButton("Close", dialog);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #1F1724; border: none; border-radius: 6px; padding: 10px; color: white; font-weight: bold; }"
        "QPushButton:hover { background-color: #7C3E66; }"
        );
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);

    QHBoxLayout *footerLayout = new QHBoxLayout();
    footerLayout->addStretch();
    footerLayout->addWidget(closeBtn);
    mainLayout->addLayout(footerLayout);

    dialog->exec();
    dialog->deleteLater();
}

void AdminPanel::showBookDetailsDialog(const QJsonObject &data)
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Book Details");
    dialog->resize(560, 520);
    dialog->setStyleSheet("background-color: #09070C; color: #EAEAEA; font-family: 'Segoe UI', Arial;");

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    mainLayout->setContentsMargins(24, 24, 24, 20);
    mainLayout->setSpacing(14);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(16);

    QLabel *cover = new QLabel("📖", dialog);
    cover->setFixedSize(64, 64);
    cover->setAlignment(Qt::AlignCenter);
    cover->setStyleSheet("font-size: 30px; background-color: #1F1724; border-radius: 32px;");

    QVBoxLayout *headerTextLayout = new QVBoxLayout();
    headerTextLayout->setSpacing(2);

    QLabel *titleLabel = new QLabel(data["title"].toString(), dialog);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #FFFFFF;");

    QLabel *authorLabel = new QLabel("by " + data["author"].toString(), dialog);
    authorLabel->setStyleSheet("font-size: 12px; color: #9A8FA0;");

    bool isActive = data["isActive"].toBool();
    QLabel *statusBadge = new QLabel(isActive ? "✅ APPROVED" : "🚫 PENDING/REJECTED", dialog);
    statusBadge->setStyleSheet(
        QString("background-color: %1; color: white; font-size: 10px; font-weight: bold; "
                "padding: 3px 10px; border-radius: 8px; letter-spacing: 1px;")
            .arg(isActive ? "#268730" : "#8d1f1f"));
    statusBadge->setFixedHeight(20);
    statusBadge->setMaximumWidth(220);

    headerTextLayout->addWidget(titleLabel);
    headerTextLayout->addWidget(authorLabel);
    headerTextLayout->addSpacing(4);
    headerTextLayout->addWidget(statusBadge);

    headerLayout->addWidget(cover);
    headerLayout->addLayout(headerTextLayout, 1);
    mainLayout->addLayout(headerLayout);

    QString cardStyle =
        "QGroupBox { border: 1px solid #1F1724; border-radius: 10px; margin-top: 8px; padding-top: 16px; "
        "color: #A594B3; font-weight: bold; background-color: #0F0C12; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }";

    QGroupBox *infoBox = new QGroupBox("Book Information", dialog);
    infoBox->setStyleSheet(cardStyle);
    QFormLayout *formLayout = new QFormLayout(infoBox);
    formLayout->setLabelAlignment(Qt::AlignRight);
    formLayout->setHorizontalSpacing(16);
    formLayout->setVerticalSpacing(8);

    QString labelStyle = "color: #9A8FA0; font-size: 13px;";
    QString valueStyle = "color: #EAEAEA; font-size: 14px; font-weight: bold;";

    auto addRow = [&](const QString &title, const QString &value) {
        QLabel *lbl = new QLabel(title);
        lbl->setStyleSheet(labelStyle);
        QLabel *val = new QLabel(value.isEmpty() ? "—" : value);
        val->setStyleSheet(valueStyle);
        val->setWordWrap(true);
        formLayout->addRow(lbl, val);
    };

    addRow("Genre:", data["genre"].toString());
    addRow("Price:", "$" + QString::number(data["price"].toDouble(), 'f', 2));
    double rating = data["averageRating"].toDouble();
    addRow("Avg Rating:", rating > 0.0 ? QString::number(rating, 'f', 1) + " ⭐" : "N/A");
    addRow("Description:", data["description"].toString());

    mainLayout->addWidget(infoBox);
    mainLayout->addStretch();

    QPushButton *closeDetailsBtn = new QPushButton("Close", dialog);
    closeDetailsBtn->setCursor(Qt::PointingHandCursor);
    closeDetailsBtn->setStyleSheet(
        "QPushButton { background-color: #1F1724; border: none; border-radius: 6px; padding: 10px; color: white; font-weight: bold; }"
        "QPushButton:hover { background-color: #7C3E66; }"
        );
    connect(closeDetailsBtn, &QPushButton::clicked, dialog, &QDialog::accept);

    QHBoxLayout *detailsFooterLayout = new QHBoxLayout();
    detailsFooterLayout->addStretch();
    detailsFooterLayout->addWidget(closeDetailsBtn);
    mainLayout->addLayout(detailsFooterLayout);

    dialog->exec();
    dialog->deleteLater();
}
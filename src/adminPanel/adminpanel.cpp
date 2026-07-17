#include "adminpanel.h"
#include "src/adminPanel/editbookdialogadmin.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFormLayout>
#include <QGroupBox>

AdminPanel::AdminPanel(QWidget *parent)
    : QWidget(parent)
{
    m_socket = new QTcpSocket(this);

    setupUi();
    switchPage(0);

    connect(m_socket,&QTcpSocket::readyRead,this,&AdminPanel::onReadyRead);
    m_socket->connectToHost("127.0.0.1" , 1234);

    connect(m_socket, &QTcpSocket::connected, this, [this]() {

        m_userTab->refreshTable();
        m_publishersTab->refreshTable();

        QJsonObject reqBooks;
        reqBooks["action"] = "get_books_list";
        m_socket->write(QJsonDocument(reqBooks).toJson(QJsonDocument::Compact) + "\n");

        QJsonObject reqReviews;
        reqReviews["action"] = "get_all_reviews_admin";
        m_socket->write(QJsonDocument(reqReviews).toJson(QJsonDocument::Compact) + "\n");
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
    m_btnReviews = new QPushButton("🛡️ Moderate Reviews", sidebar);

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
    m_btnReviews->setStyleSheet(menuBtnStyle);

    m_btnMonitor->setCursor(Qt::PointingHandCursor);
    m_btnUsers->setCursor(Qt::PointingHandCursor);
    m_btnBooks->setCursor(Qt::PointingHandCursor);
    m_btnPublishers->setCursor(Qt::PointingHandCursor);
    m_btnReviews->setCursor(Qt::PointingHandCursor);

    sidebarLayout->addWidget(m_btnMonitor);
    sidebarLayout->addWidget(m_btnUsers);
    sidebarLayout->addWidget(m_btnPublishers);
    sidebarLayout->addWidget(m_btnBooks);
    sidebarLayout->addWidget(m_btnReviews);

    sidebarLayout->addStretch();
    sidebarLayout->addWidget(m_btnLogout);

    m_stackedWidget = new QStackedWidget(this);

    ServerWindow *serverPage = new ServerWindow(this);
    m_stackedWidget->addWidget(serverPage);

    m_userTab = new UsersTab(m_socket,this);
    m_publishersTab = new PublishersTab(m_socket,this);

    m_stackedWidget->addWidget(m_userTab);
    m_stackedWidget->addWidget(m_publishersTab);

    QWidget *booksPage = createBooksPage();
    m_stackedWidget->addWidget(booksPage);

    QWidget *reviewsPage = createReviewsPage();
    m_stackedWidget->addWidget(reviewsPage);

    mainLayout->addWidget(m_stackedWidget);
    mainLayout->addWidget(sidebar);

    connect(m_btnMonitor, &QPushButton::clicked, this, [this](){ switchPage(0); });
    connect(m_btnUsers, &QPushButton::clicked, this, [this](){ switchPage(1); });
    connect(m_btnPublishers, &QPushButton::clicked, this, [this](){ switchPage(2); });
    connect(m_btnBooks, &QPushButton::clicked, this, [this](){ switchPage(3); });
    connect(m_btnReviews, &QPushButton::clicked, this, [this](){ switchPage(4); });
    connect(m_btnLogout, &QPushButton::clicked, this, &AdminPanel::logoutRequested);
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

QWidget* AdminPanel::createReviewsPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(12);

    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->setSpacing(10);

    m_reviewSearchEdit = new QLineEdit(page);
    m_reviewSearchEdit->setPlaceholderText("🔍 Search reviews by book title or username...");
    m_reviewSearchEdit->setStyleSheet(
        "QLineEdit { background-color: #120E14; border: 1px solid #1F1724; border-radius: 6px; "
        "padding: 8px; color: #EAEAEA; font-size: 13px; }"
        "QLineEdit:focus { border: 1px solid #7C3E66; }"
        );

    m_reviewFilterCombo = new QComboBox(page);
    m_reviewFilterCombo->addItem("All Reviews");
    m_reviewFilterCombo->addItem("Pending Approval");
    m_reviewFilterCombo->setStyleSheet(
        "QComboBox { background-color: #120E14; border: 1px solid #1F1724; border-radius: 6px; "
        "padding: 8px; color: #EAEAEA; font-size: 13px; min-width: 160px; }"
        "QComboBox::drop-down { border: none; }"
        );

    topBar->addWidget(m_reviewSearchEdit, 1);
    topBar->addWidget(m_reviewFilterCombo);
    layout->addLayout(topBar);

    m_reviewsTable = new QTableWidget(page);
    m_reviewsTable->setColumnCount(7);
    m_reviewsTable->setHorizontalHeaderLabels({"ID", "Book Title", "Username", "Rating", "Comment", "Date", "Status"});
    m_reviewsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_reviewsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_reviewsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_reviewsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_reviewsTable->verticalHeader()->setVisible(false);

    m_reviewsTable->setStyleSheet(
        "QTableWidget { background-color: #09070C; border: 1px solid #1F1724; gridline-color: #1F1724; color: #EAEAEA; }"
        "QTableWidget::item:selected { background-color: #7C3E66; color: white; }"
        "QHeaderView::section { background-color: #120E14; color: #A594B3; font-weight: bold; border: 1px solid #1F1724; padding: 6px; }"
        );
    layout->addWidget(m_reviewsTable);

    QHBoxLayout *reviewBtnLayout = new QHBoxLayout();
    reviewBtnLayout->setSpacing(10);

    m_btnApproveReview = new QPushButton("✅ Approve Review", page);
    m_btnApproveReview->setCursor(Qt::PointingHandCursor);
    m_btnApproveReview->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #268730; border-radius: 6px; "
        "padding: 8px; font-weight: bold; color: #7FDB8F; }"
        "QPushButton:hover { background-color: rgba(38, 135, 48, 50); color: white; }"
        );

    m_btnDeleteReview = new QPushButton("🗑️ Delete Review", page);
    m_btnDeleteReview->setCursor(Qt::PointingHandCursor);
    m_btnDeleteReview->setStyleSheet(m_btnDeleteBook->styleSheet());

    reviewBtnLayout->addStretch();
    reviewBtnLayout->addWidget(m_btnDeleteReview);
    reviewBtnLayout->addWidget(m_btnApproveReview);
    layout->addLayout(reviewBtnLayout);

    connect(m_reviewSearchEdit, &QLineEdit::textChanged, this, &AdminPanel::filterReviews);
    connect(m_reviewFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),this, &AdminPanel::handleReviewFilterChanged);
    connect(m_btnApproveReview, &QPushButton::clicked, this, &AdminPanel::handleApproveReview);
    connect(m_btnDeleteReview, &QPushButton::clicked, this, &AdminPanel::handleDeleteReview);

    return page;
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



void AdminPanel::refreshBooksTable() {
    QJsonObject request;
    request["action"] = "get_books_list";
    m_socket->write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");
    m_socket->flush();
}

void AdminPanel::refreshReviewsTable()
{
    QJsonObject request;
    request["action"] = m_reviewShowPendingOnly ? "get_pending_reviews" : "get_all_reviews_admin";
    m_socket->write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");
    m_socket->flush();
}

void AdminPanel::switchPage(int index)
{
    m_stackedWidget->setCurrentIndex(index);
    updateButtonStyles(index);
}

void AdminPanel::handleReviewFilterChanged(int index)
{
    m_reviewShowPendingOnly = (index == 1);
    refreshReviewsTable();
}

void AdminPanel::filterReviews(const QString &text)
{
    for (int i = 0; i < m_reviewsTable->rowCount(); ++i) {
        QString bookTitle = m_reviewsTable->item(i, 1)->text();
        QString username = m_reviewsTable->item(i, 2)->text();
        bool match = bookTitle.contains(text, Qt::CaseInsensitive) ||
                     username.contains(text, Qt::CaseInsensitive);
        m_reviewsTable->setRowHidden(i, !match);
    }
}

void AdminPanel::populateReviewsTable(const QJsonArray &reviews, bool pendingOnly)
{
    m_reviewsTable->setRowCount(0);
    for (int i = 0; i < reviews.size(); ++i) {
        QJsonObject r = reviews[i].toObject();
        m_reviewsTable->insertRow(i);

        m_reviewsTable->setItem(i, 0, new QTableWidgetItem(QString::number(r["id"].toInt())));
        m_reviewsTable->setItem(i, 1, new QTableWidgetItem(r["bookTitle"].toString()));
        m_reviewsTable->setItem(i, 2, new QTableWidgetItem(r["username"].toString()));
        m_reviewsTable->setItem(i, 3, new QTableWidgetItem(QString::number(r["rating"].toInt()) + " ⭐"));
        m_reviewsTable->setItem(i, 4, new QTableWidgetItem(r["comment"].toString()));
        m_reviewsTable->setItem(i, 5, new QTableWidgetItem(r["date"].toString()));

        bool isApproved = pendingOnly ? false : r["isApproved"].toBool();
        QTableWidgetItem *statusItem = new QTableWidgetItem(isApproved ? "✅ Approved" : "⏳ Pending");
        statusItem->setForeground(isApproved ? QColor("#7FDB8F") : QColor("#F5D7A0"));
        m_reviewsTable->setItem(i, 6, statusItem);

        m_reviewsTable->item(i, 0)->setData(Qt::UserRole, r["bookId"].toInt());
        m_reviewsTable->item(i, 2)->setData(Qt::UserRole, r["userId"].toInt());
    }
}

void AdminPanel::handleApproveReview()
{
    int currentRow = m_reviewsTable->currentRow();
    if (currentRow < 0) return;

    int reviewId = m_reviewsTable->item(currentRow, 0)->text().toInt();
    int bookId = m_reviewsTable->item(currentRow, 0)->data(Qt::UserRole).toInt();
    int userId = m_reviewsTable->item(currentRow, 2)->data(Qt::UserRole).toInt();
    QString bookTitle = m_reviewsTable->item(currentRow, 1)->text();

    QJsonObject packet;
    packet["action"] = "approve_review";
    packet["reviewId"] = reviewId;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void AdminPanel::handleDeleteReview()
{
    int currentRow = m_reviewsTable->currentRow();
    if (currentRow < 0) return;

    int reviewId = m_reviewsTable->item(currentRow, 0)->text().toInt();
    QString bookTitle = m_reviewsTable->item(currentRow, 1)->text();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirm Delete",
        "Are you sure you want to remove this review for \"" + bookTitle + "\"?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QJsonObject packet;
        packet["action"] = "review_delete";
        packet["reviewId"] = reviewId;
        m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
    }
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
    m_btnReviews->setStyleSheet(currentIndex == 4 ? activeStyle : normalStyle);
}

void AdminPanel::mousePressEvent(QMouseEvent *event)
{
    m_userTab->clearTableSelection();
    m_booksTable->clearSelection();
    m_publishersTab->clearTableSelection();
    m_reviewsTable->clearSelection();
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

        m_userTab->handleServerResponse(response);
        m_publishersTab->handleServerResponse(response);

        if (action == "books_list_response" && response["status"] == "success") {
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
        else if (action == "delete_book_response" && response["status"] == "success") {
            int currentRow = m_booksTable->currentRow();
            if (currentRow >= 0) {
                m_booksTable->removeRow(currentRow);
            }
            QMessageBox::information(this, "Success", "Book completely deleted from the database.");
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
        else if (action == "all_reviews_response" && response["status"].toString() == "success") {
            populateReviewsTable(response["data"].toArray(), false);
        }
        else if (action == "pending_reviews_response" && response["status"].toString() == "success") {
            populateReviewsTable(response["data"].toArray(), true);
        }
        else if (action == "approve_review_response") {
            if (response["status"].toString() == "success") {
                refreshReviewsTable();
            } else {
                QMessageBox::warning(this, "Approve Failed", response["message"].toString());
            }
        }
        else if (action == "review_delete_response") {
            if (response["status"].toString() == "success") {
                QMessageBox::information(this, "Success", "Review removed successfully.");
                refreshReviewsTable();
            } else {
                QMessageBox::warning(this, "Delete Failed", response["message"].toString());
            }
        }
    }
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
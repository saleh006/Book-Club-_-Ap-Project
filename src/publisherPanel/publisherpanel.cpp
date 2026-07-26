#include "publisherpanel.h"
#include "bookcardwidget.h"
#include "addeditbookdialog.h"
#include "setofferdialog.h"
#include "editprofiledialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QValueAxis>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QHeaderView>
#include <QScrollArea>
#include <QGridLayout>
#include <QTimer>
#include <QCoreApplication>
#include "styledmessagebox.h"

static const char *kCardBg     = "#120E14";
static const char *kCardBorder = "#1F1724";
static const char *kAccent     = "#A85CF0";
static const char *kTextDim    = "#9A8FA0";
static const char *kGrid       = "#2A2233";

PublisherPanel::PublisherPanel(int publisherId, const QString &fullName, const QString &username, QWidget *parent)
    : QWidget(parent), m_publisherId(publisherId), m_fullName(fullName), m_username(username)
{
    setupUi();

    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::readyRead, this, &PublisherPanel::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &PublisherPanel::onSocketError);
    connect(m_socket, &QTcpSocket::connected, this, [this]() {
        QJsonObject subReq;
        subReq["action"] = "user_subscribe";
        subReq["userId"] = m_publisherId;
        sendRequest(subReq);

        requestNotifications();

        requestBooks();
        requestStats();
        requestSalesTrend();
        QJsonObject req;
        req["action"] = "user_fetch";
        req["username"] = m_username;
        sendRequest(req);
    });
    m_socket->connectToHost("127.0.0.1", 1234);

    switchPage(0); // default to "My Books"
}

void PublisherPanel::sendRequest(const QJsonObject &requestObj)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        StyledMessageBox::error(this, "Not connected", "Not connected to the server.");
        return;
    }
    m_socket->write(QJsonDocument(requestObj).toJson(QJsonDocument::Compact) + "\n");
}

void PublisherPanel::requestBooks()
{
    qDebug() << "Requesting books";
    QJsonObject req;
    req["action"] = "publisher_get_books";
    req["publisherId"] = m_publisherId;
    sendRequest(req);
}

void PublisherPanel::requestStats()
{
    QJsonObject req;
    req["action"] = "publisher_get_stats";
    req["publisherId"] = m_publisherId;
    sendRequest(req);
}

void PublisherPanel::setupUi()
{
    setStyleSheet("background-color: #060508; color: #EAEAEA; font-family: 'Segoe UI', Arial;");
    this->resize(800, 500);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget *sidebar = new QWidget(this);
    sidebar->setFixedWidth(220);
    sidebar->setStyleSheet("background-color: #120E14; border-left: 1px solid #1F1724;");

    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(15, 25, 15, 25);
    sidebarLayout->setSpacing(12);

    QWidget *avatarRow = new QWidget(sidebar);
    avatarRow->setStyleSheet("background: transparent; border: none;");
    QHBoxLayout *avatarRowLayout = new QHBoxLayout(avatarRow);
    avatarRowLayout->setContentsMargins(0, 0, 0, 0);
    avatarRowLayout->setSpacing(8);

    QLabel *avatarLabel = new QLabel("📖", avatarRow);
    avatarLabel->setStyleSheet("font-size: 40px; border: none; background: transparent;");

    QPushButton *editProfileBtn = new QPushButton(avatarRow);

    editProfileBtn->setIcon(QIcon(":/icons/pen.png"));
    editProfileBtn->setIconSize(QSize(18, 18));
    editProfileBtn->setFixedSize(30, 30);
    editProfileBtn->setCursor(Qt::PointingHandCursor);
    editProfileBtn->setToolTip("Edit profile");
    editProfileBtn->setStyleSheet(
        "QPushButton { background-color: #1F1724; border: 1px solid #2A2233; border-radius: 8px; font-size: 13px; }"
        "QPushButton:hover { background-color: #7C3E66; border-color: #B06B96; }");
    connect(editProfileBtn, &QPushButton::clicked, this, &PublisherPanel::handleEditProfile);

    avatarRowLayout->addStretch();
    avatarRowLayout->addWidget(avatarLabel);
    avatarRowLayout->addWidget(editProfileBtn);
    avatarRowLayout->addStretch();

    QLabel *roleLabel = new QLabel("Publisher", sidebar);
    roleLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #FFEAD2; border: none; background: transparent;");
    roleLabel->setAlignment(Qt::AlignCenter);

    m_nameLabel = new QLabel(m_fullName.isEmpty() ? m_username : m_fullName, sidebar);
    m_nameLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #EAEAEA; border: none; background: transparent;");
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setWordWrap(true);

    m_usernameLabel = new QLabel("@" + m_username, sidebar);
    m_usernameLabel->setStyleSheet("font-size: 11px; color: #9A8FA0; border: none; background: transparent;");
    m_usernameLabel->setAlignment(Qt::AlignCenter);

    sidebarLayout->addWidget(m_nameLabel);
    sidebarLayout->addWidget(m_usernameLabel);
    sidebarLayout->addSpacing(8);
    sidebarLayout->addWidget(avatarRow);
    sidebarLayout->addWidget(roleLabel);
    sidebarLayout->addSpacing(15);


    m_btnStats = new QPushButton("Book Statistics", sidebar);

    m_btnStats->setIcon(QIcon(":/icons/chart-area-solid.png"));
    m_btnStats->setIconSize(QSize(20, 20));
    m_btnBooks = new QPushButton("My Books", sidebar);
    m_btnBooks->setIcon(QIcon(":/icons/book.png"));
    m_btnBooks->setIconSize(QSize(20, 20));
    m_btnNotifications = new QPushButton("Notifications", sidebar);
    m_btnNotifications->setIcon(QIcon(":/icons/bell.png"));
    m_btnNotifications->setIconSize(QSize(20, 20));
    m_btnLogout = new QPushButton("Logout", sidebar);
    m_btnLogout->setIcon(QIcon(":/icons/logout.png"));
    m_btnLogout->setIconSize(QSize(20, 20));

    QString menuBtnStyle =
        "QPushButton { background-color: transparent; border: none; border-radius: 8px; padding: 10px; font-size: 13px; color: #9A8FA0; text-align: left; padding-left: 12px; }"
        "QPushButton:hover { background-color: #1F1724; color: #EAEAEA; }";

    m_btnStats->setStyleSheet(menuBtnStyle);
    m_btnBooks->setStyleSheet(menuBtnStyle);
    m_btnNotifications->setStyleSheet(menuBtnStyle);
    m_btnStats->setCursor(Qt::PointingHandCursor);
    m_btnBooks->setCursor(Qt::PointingHandCursor);
    m_btnNotifications->setCursor(Qt::PointingHandCursor);

    m_btnLogout->setCursor(Qt::PointingHandCursor);
    m_btnLogout->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #7C3E66; border-radius: 8px; padding: 8px; font-weight: bold; color: #D9C2D1; text-align: left; padding-left: 12px; }"
        "QPushButton:hover { background-color: rgba(124, 62, 102, 60); color: white; border: 1px solid #B06B96; }"
        );


    m_btnNotifications->setCursor(Qt::PointingHandCursor);

    auto *notifBtnLayout = new QGridLayout(m_btnNotifications);
    notifBtnLayout->setContentsMargins(0, 3, 6, 0);
    notifBtnLayout->setColumnStretch(0, 1);
    notifBtnLayout->setRowStretch(1, 1);

    m_notifBadge = new QLabel(m_btnNotifications);
    m_notifBadge->setAlignment(Qt::AlignCenter);
    m_notifBadge->setFixedHeight(18);
    m_notifBadge->setMinimumWidth(18);
    m_notifBadge->setStyleSheet(
        "QLabel { background-color: #e46060; color: white; font-size: 10px; font-weight: 700;"
        " border-radius: 9px; padding: 0 4px; }");
    m_notifBadge->hide();
    notifBtnLayout->addWidget(m_notifBadge, 0, 1, Qt::AlignTop | Qt::AlignRight);


    sidebarLayout->addWidget(m_btnStats);
    sidebarLayout->addWidget(m_btnBooks);
    sidebarLayout->addWidget(m_btnNotifications);
    sidebarLayout->addStretch();
    sidebarLayout->addWidget(m_btnLogout);

    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->addWidget(createStatsPage()); // index 0
    m_stackedWidget->addWidget(createBooksPage()); // index 1

    m_notifPage = createNotificationsPage();
    m_stackedWidget->addWidget(m_notifPage);

    mainLayout->addWidget(m_stackedWidget);
    mainLayout->addWidget(sidebar);

    connect(m_btnStats, &QPushButton::clicked, this, [this]() { switchPage(0); requestStats(); });
    connect(m_btnBooks, &QPushButton::clicked, this, [this]() { switchPage(1); });
    connect(m_btnNotifications, &QPushButton::clicked, this, [this]() { switchPage(2); });
    connect(m_btnLogout, &QPushButton::clicked, this, &PublisherPanel::logoutRequested);
}

// QWidget* PublisherPanel::createStatsPage()
// {
//     QWidget *page = new QWidget(this);
//     QVBoxLayout *layout = new QVBoxLayout(page);
//     layout->setContentsMargins(30, 30, 30, 30);
//     layout->setSpacing(16);

//     auto makeStatCard = [page](const QString &title, QLabel *&valueLabelOut) -> QWidget* {
//         QWidget *card = new QWidget(page);
//         card->setStyleSheet("background-color: #120E14; border: 1px solid #1F1724; border-radius: 10px;");
//         QVBoxLayout *cardLayout = new QVBoxLayout(card);
//         QLabel *titleLabel = new QLabel(title, card);
//         titleLabel->setStyleSheet("color: #A594B3; font-size: 12px;");
//         valueLabelOut = new QLabel("—", card);
//         valueLabelOut->setStyleSheet("color: #EAEAEA; font-size: 24px; font-weight: bold;");
//         cardLayout->addWidget(titleLabel);
//         cardLayout->addWidget(valueLabelOut);
//         return card;
//     };

//     QGridLayout *statsGrid = new QGridLayout();
//     statsGrid->setSpacing(16);
//     statsGrid->addWidget(makeStatCard("Total Books", m_statBookCount), 0, 0);
//     statsGrid->addWidget(makeStatCard("Total Sales", m_statTotalSales), 0, 1);
//     statsGrid->addWidget(makeStatCard("Average Rating", m_statAvgRating), 1, 0);
//     statsGrid->addWidget(makeStatCard("Total Income", m_statTotalIncome), 1, 1);

//     layout->addLayout(statsGrid);
//     layout->addStretch();

//     return page;
// }

QWidget* PublisherPanel::createBooksPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    m_bookSearchEdit = new QLineEdit(page);
    m_bookSearchEdit->setPlaceholderText("Search your books by title...");

    QAction *searchAction = new QAction(QIcon(":/icons/magnifying-glass-solid.png"), "", m_bookSearchEdit);
    m_bookSearchEdit->addAction(searchAction, QLineEdit::LeadingPosition);
    m_bookSearchEdit->setStyleSheet(
        "QLineEdit { background-color: #120E14; border: 1px solid #1F1724; border-radius: 6px; "
        "padding: 8px; color: #EAEAEA; font-size: 13px; }"
        "QLineEdit:focus { border: 1px solid #7C3E66; }"
        );
    layout->addWidget(m_bookSearchEdit);

    m_booksScrollArea = new QScrollArea(page);
    m_booksScrollArea->setWidgetResizable(true);
    m_booksScrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");

    m_booksGridContainer = new QWidget(m_booksScrollArea);
    m_booksGridContainer->setStyleSheet("background: transparent;");

    m_booksGrid = new QGridLayout(m_booksGridContainer);
    m_booksGrid->setSpacing(16);
    m_booksGrid->setContentsMargins(0, 0, 0, 0);

    m_booksScrollArea->setWidget(m_booksGridContainer);
    layout->addWidget(m_booksScrollArea);

    connect(m_bookSearchEdit, &QLineEdit::textChanged, this, &PublisherPanel::filterBooks);

    populateBooksGrid(QVector<Book>());

    return page;
}

void PublisherPanel::populateBooksGrid(const QVector<Book> &books)
{
    m_currentBooks = books;

    QLayoutItem *item;
    while ((item = m_booksGrid->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    const int columns = 5;
    int row = 0, col = 0;

    for (const Book &book : std::as_const(books)) {
        if (book.status == -1) {
            continue;
        }
        BookCardWidget *card = new BookCardWidget(book, m_booksGridContainer);
        connect(card, &BookCardWidget::editRequested, this, &PublisherPanel::handleEditBook);
        connect(card, &BookCardWidget::deleteRequested, this, &PublisherPanel::handleDeleteBook);
        connect(card, &BookCardWidget::offerRequested, this, &PublisherPanel::handleSetOffer);
        connect(card, &BookCardWidget::toggleActiveRequested, this, &PublisherPanel::handleToggleActive);
        m_booksGrid->addWidget(card, row, col);
        col++;
        if (col >= columns) { col = 0; row++; }
    }

    QPushButton *addCard = new QPushButton("+", m_booksGridContainer);
    addCard->setFixedSize(180, 240);
    addCard->setCursor(Qt::PointingHandCursor);
    addCard->setStyleSheet(
        "QPushButton { background-color: transparent; border: 2px dashed #5F2E4F; border-radius: 8px; font-size: 32px; color: #7C3E66; }"
        "QPushButton:hover { background-color: rgba(124, 62, 102, 30); color: #B06B96; border: 2px dashed #7C3E66; }"
        );
    connect(addCard, &QPushButton::clicked, this, &PublisherPanel::handleAddBook);

    m_booksGrid->addWidget(addCard, row, col);
}

void PublisherPanel::handleAddBook()
{
    Book emptyBook;
    emptyBook.publisherId = m_publisherId;

    AddEditBookDialogPub dialog(emptyBook, this);
    if (dialog.exec() == QDialog::Accepted) {
        Book b = dialog.resultBook();
        QJsonObject req;
        req["action"] = "publisher_add_book";
        req["publisherId"] = m_publisherId;
        req["title"] = b.title;
        req["author"] = b.author;
        req["genre"] = b.genre;
        req["description"] = b.description;
        req["price"] = b.price;
        req["coverImagePath"] = b.coverImagePath;
        req["pdfPath"] = b.pdfPath;
        sendRequest(req);
    }
}

void PublisherPanel::handleEditBook(int bookId)
{
    auto it = std::find_if(m_currentBooks.begin(), m_currentBooks.end(),
                           [bookId](const Book &b) { return b.id == bookId; });
    if (it == m_currentBooks.end()) return;

    AddEditBookDialogPub dialog(*it, this);
    if (dialog.exec() == QDialog::Accepted) {
        Book b = dialog.resultBook();
        QJsonObject req;
        req["action"] = "publisher_update_book";
        req["id"] = b.id;
        req["publisherId"] = m_publisherId;
        req["title"] = b.title;
        req["author"] = b.author;
        req["genre"] = b.genre;
        req["description"] = b.description;
        req["price"] = b.price;
        req["coverImagePath"] = b.coverImagePath;
        req["pdfPath"] = b.pdfPath;
        sendRequest(req);
    }
}

void PublisherPanel::handleDeleteBook(int bookId)
{
    bool confirmed = StyledMessageBox::question(this, "Delete Book",
                                                "Are you sure you want to remove this book? It will be hidden from readers.");
    if (!confirmed) return;

    QJsonObject req;
    req["action"] = "publisher_delete_book";
    req["bookId"] = bookId;
    sendRequest(req);
}

void PublisherPanel::handleToggleActive(int bookId, int newStatus)
{
    QJsonObject req;
    req["action"] = "publisher_set_book_status";
    req["bookId"] = bookId;
    req["publisherId"] = m_publisherId;
    req["status"] = newStatus;
    sendRequest(req);
}

void PublisherPanel::handleSetOffer(int bookId)
{
    SetOfferDialog dialog(bookId, this);
    if (dialog.exec() == QDialog::Accepted) {
        Discount d = dialog.resultDiscount();

        if (d.startDate >= d.endDate) {
            StyledMessageBox::error(this, "Invalid dates", "Start time must be before end time.");
            return;
        }

        QJsonObject req;
        req["action"] = "publisher_add_discount";
        req["publisherId"] = m_publisherId;
        req["bookId"] = d.bookId;
        req["type"] = d.type;
        req["value"] = d.value;
        req["startDate"] = d.startDate.toUTC().toString(Qt::ISODate);
        req["endDate"] = d.endDate.toUTC().toString(Qt::ISODate);
        sendRequest(req);
    }
}

void PublisherPanel::requestNotifications()
{
    QJsonObject req;
    req["action"] = "notifications_fetch";
    req["userId"] = m_publisherId;
    sendRequest(req);
}

QWidget *PublisherPanel::createNotificationsPage()
{
    auto *page = new QWidget(this);
    page->setStyleSheet(QString("background-color:%1;border:none;").arg(kNotifPanelBg));
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 26, 30, 30);
    layout->setSpacing(14);

    auto *headerLayout = new QHBoxLayout();
    auto *titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);
    auto *title = new QLabel("Notifications", page);
    title->setStyleSheet("color:#FFFFFF;font-size:21px;font-weight:700;border:none;background:transparent;");
    auto *subtitle = new QLabel("What's new with your books and publisher account", page);
    subtitle->setStyleSheet(QString("color:%1;font-size:12px;border:none;background:transparent;").arg(kNotifTextDim));
    titleCol->addWidget(title);
    titleCol->addWidget(subtitle);

    auto *markAllBtn = new QPushButton("✓ Mark all as read", page);
    markAllBtn->setCursor(Qt::PointingHandCursor);
    markAllBtn->setStyleSheet(
        "QPushButton{color:#CFC7D6;font-size:12px;font-weight:600;border:1px solid #34303F;"
        "border-radius:8px;background-color:#1E1B26;padding:8px 14px;}"
        "QPushButton:hover{background-color:#2A2635;border-color:#443F52;color:#FFFFFF;}"
        "QPushButton:pressed{background-color:#242030;}");
    connect(markAllBtn, &QPushButton::clicked, this, [this]() {
        QJsonObject req;
        req["action"] = "notifications_mark_all_read";
        req["userId"] = m_publisherId;
        sendRequest(req);
    });

    auto *clearAllBtn = new QPushButton("🗑 Clear all", page);
    clearAllBtn->setCursor(Qt::PointingHandCursor);
    clearAllBtn->setStyleSheet(
        "QPushButton{color:#E4577A;font-size:12px;font-weight:600;border:1px solid #3A2430;"
        "border-radius:8px;background-color:#1E1B26;padding:8px 14px;}"
        "QPushButton:hover{background-color:#2A1A22;border-color:#E4577A;color:#FF7A9C;}"
        "QPushButton:pressed{background-color:#241018;}");
    connect(clearAllBtn, &QPushButton::clicked, this, [this]() {
        QJsonObject req;
        req["action"] = "notifications_clear_all";
        req["userId"] = m_publisherId;
        sendRequest(req);
    });

    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();
    headerLayout->addWidget(markAllBtn,0,Qt::AlignTop);
    headerLayout->addWidget(clearAllBtn,0,Qt::AlignTop);
    layout->addLayout(headerLayout);

    m_notifListWidget = new QListWidget(page);
    m_notifListWidget->setFrameShape(QFrame::NoFrame);
    m_notifListWidget->setSpacing(8);
    m_notifListWidget->setUniformItemSizes(false);
    m_notifListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_notifListWidget->setStyleSheet(
        "QListWidget{background-color:transparent;border:none;}"
        "QListWidget::item{border:none;padding:0;}"
        "QListWidget::item:selected{background:transparent;}"
        "QScrollBar:vertical{background:transparent;width:8px;margin:0;}"
        "QScrollBar::handle:vertical{background:#2E2A38;border-radius:4px;min-height:24px;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}");

    connect(m_notifListWidget, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        int notifId = item->data(Qt::UserRole).toInt();
        bool alreadyRead = item->data(Qt::UserRole + 1).toBool();
        if (alreadyRead) return;

        QJsonObject req;
        req["action"] = "notification_mark_read";
        req["userId"] = m_publisherId;
        req["notificationId"] = notifId;
        sendRequest(req);
    });
    layout->addWidget(m_notifListWidget, 1);

    return page;
}

void PublisherPanel::rebuildNotificationList()
{
    if (!m_notifListWidget) return;
    m_notifListWidget->clear();

    if (m_notifications.isEmpty()) {
        auto *empty = new QWidget(m_notifListWidget);
        auto *emptyLayout = new QVBoxLayout(empty);
        emptyLayout->setContentsMargins(0, 60, 0, 40);
        emptyLayout->setSpacing(8);
        auto *icon = new QLabel("📭", empty);
        icon->setStyleSheet("font-size:34px;border:none;background:transparent;");
        icon->setAlignment(Qt::AlignCenter);
        auto *text = new QLabel("You're all caught up", empty);
        text->setStyleSheet(QString("color:%1;font-size:13px;border:none;background:transparent;").arg(kNotifTextDim));
        text->setAlignment(Qt::AlignCenter);
        emptyLayout->addWidget(icon);
        emptyLayout->addWidget(text);

        auto *item = new QListWidgetItem();
        item->setFlags(Qt::NoItemFlags);
        item->setSizeHint(QSize(0, 160));
        m_notifListWidget->addItem(item);
        m_notifListWidget->setItemWidget(item, empty);
        return;
    }

    for (const Notification &n : m_notifications) {
        const NotifVisual visual = notificationVisualFor(n.title);

        auto *row = new NotifRowFrame(m_notifListWidget);
        row->setObjectName("notifCard");
        row->setAttribute(Qt::WA_Hover, true);
        row->setCursor(n.isRead ? Qt::ArrowCursor : Qt::PointingHandCursor);
        row->setStyleSheet(QString(
                               "QFrame#notifCard{background-color:%1;border:1px solid %2;border-radius:12px;}"
                               "QFrame#notifCard:hover{background-color:%3;border-color:%4;}")
                               .arg(n.isRead ? kNotifCardBg : kNotifUnreadBg,
                                    n.isRead ? kNotifCardBorder : visual.accent + "88",
                                    n.isRead ? "#211E29" : "#2A2436",
                                    visual.accent));

        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(14, 13, 16, 13);
        rowLayout->setSpacing(12);

        auto *accentBar = new QLabel(row);
        accentBar->setFixedWidth(3);
        accentBar->setMinimumHeight(1);
        accentBar->setStyleSheet(QString("background-color:%1;border-radius:2px;")
                                     .arg(n.isRead ? "transparent" : visual.accent));

        auto *iconBubble = new QLabel(visual.icon, row);
        iconBubble->setFixedSize(36, 36);
        iconBubble->setAlignment(Qt::AlignCenter);
        iconBubble->setStyleSheet(QString(
                                      "font-size:16px;border-radius:18px;background-color:%1;border:none;")
                                      .arg(visual.accent + "26"));

        auto *textCol = new QVBoxLayout();
        textCol->setSpacing(3);

        auto *topRow = new QHBoxLayout();
        topRow->setSpacing(8);
        auto *titleLbl = new QLabel(n.title, row);
        titleLbl->setStyleSheet(QString("color:%1;font-size:13px;font-weight:%2;border:none;background:transparent;")
                                    .arg(n.isRead ? "#C9C3D1" : "#FFFFFF", n.isRead ? "500" : "700"));
        titleLbl->setWordWrap(true);
        auto *categoryTag = new QLabel(visual.label.toUpper(), row);
        categoryTag->setStyleSheet(QString(
                                       "color:%1;font-size:9px;font-weight:700;letter-spacing:0.5px;border:none;background:transparent;")
                                       .arg(visual.accent));
        topRow->addWidget(titleLbl, 1);
        topRow->addWidget(categoryTag, 0, Qt::AlignRight | Qt::AlignTop);

        auto *msgLbl = new QLabel(n.message, row);
        msgLbl->setStyleSheet(QString("color:%1;font-size:12px;border:none;background:transparent;").arg(kNotifTextDim));
        msgLbl->setWordWrap(true);

        auto *timeLbl = new QLabel(formatRelativeNotifTime(n.date), row);
        timeLbl->setStyleSheet("color:#5C5668;font-size:10.5px;border:none;background:transparent;");

        textCol->addLayout(topRow);
        textCol->addWidget(msgLbl);
        textCol->addWidget(timeLbl);

        rowLayout->addWidget(accentBar);
        rowLayout->addWidget(iconBubble, 0, Qt::AlignTop);
        rowLayout->addLayout(textCol, 1);

        if (!n.isRead) {
            auto *unreadDot = new QLabel(row);
            unreadDot->setFixedSize(8, 8);
            unreadDot->setStyleSheet("background-color:#E4577A;border-radius:4px;");
            rowLayout->addWidget(unreadDot, 0, Qt::AlignTop);
        }

        auto *deleteBtn = new QPushButton("✕", row);
        deleteBtn->setFixedSize(22, 22);
        deleteBtn->setCursor(Qt::PointingHandCursor);
        deleteBtn->setToolTip("Delete notification");
        deleteBtn->setStyleSheet(
            "QPushButton{color:#6B7280;border:none;background:transparent;font-size:12px;border-radius:11px;}"
            "QPushButton:hover{color:#FFFFFF;background-color:#3A2430;}");
        deleteBtn->hide();

        const int notifId = n.id;
        connect(deleteBtn, &QPushButton::clicked, this, [this, notifId]() {
            QJsonObject req;
            req["action"] = "notification_delete";
            req["userId"] = m_publisherId;
            req["notificationId"] = notifId;
            sendRequest(req);
        });

        rowLayout->addWidget(deleteBtn, 0, Qt::AlignTop);
        row->hoverButton = deleteBtn;

        auto *item = new QListWidgetItem();
        item->setSizeHint(row->sizeHint());
        item->setData(Qt::UserRole, n.id);
        item->setData(Qt::UserRole + 1, n.isRead);
        m_notifListWidget->addItem(item);
        m_notifListWidget->setItemWidget(item, row);
    }
}

void PublisherPanel::updateNotificationBadge()
{
    int unread = 0;
    for (const Notification &n : m_notifications) if (!n.isRead) unread++;

    if (unread <= 0) { m_notifBadge->hide(); return; }
    m_notifBadge->setText(unread > 99 ? QStringLiteral("99+") : QString::number(unread));
    m_notifBadge->show();
}

void PublisherPanel::showNotificationToast(const QString &title, const QString &message)
{
    NotificationToast::show(this, title, message);
}

void PublisherPanel::onReadyRead()
{
    while (m_socket->canReadLine()) {

        QByteArray data = m_socket->readLine().trimmed();

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);

        if (err.error != QJsonParseError::NoError) {
            qWarning() << "JSON parse error:" << err.errorString();
            continue;
        }

        if (!doc.isObject()) {
            continue;
        }

        const QJsonObject responseObj = doc.object();
        const QString type = responseObj["type"].toString();
        const QString action = responseObj["action"].toString();

        if (action == "notify_account_blocked") {
            showBlockedOverlay();
            return;
        }

        if (type == "table_refresh_required") {
            if (responseObj["target_table"].toString() == "book") {
                requestBooks();
                requestStats();
            }
            continue;
        }

        if (action == "notify_book_updated" || action == "notify_book_removed") {
            requestBooks();
            requestStats();
            continue;
        }

        if (type == "publisher_books_list") {
            QVector<Book> books;
            const QJsonArray booksArray = responseObj["books"].toArray();
            for (const QJsonValue &val : booksArray) {
                const QJsonObject bo = val.toObject();
                Book b;
                b.id = bo["id"].toInt();
                b.publisherId = m_publisherId;
                b.title = bo["title"].toString();
                b.author = bo["author"].toString();
                b.genre = bo["genre"].toString();
                b.description = bo["description"].toString();
                b.price = bo["price"].toDouble();
                b.coverImagePath = bo["coverImagePath"].toString();
                b.pdfPath = bo["pdfPath"].toString();
                b.status = bo["status"].toInt();
                b.averageRating = bo["averageRating"].toDouble();
                b.totalSales = bo["totalSales"].toInt();
                books.push_back(b);
            }
            m_allBooks = books;
            filterBooks(m_bookSearchEdit->text());
            updateDashboard();
        }
        else if (type == "publisher_stats") {
            m_statBookCount->setText(QString::number(responseObj["bookCount"].toInt()));
            m_statTotalSales->setText(QString::number(responseObj["totalSales"].toInt()));
            m_statAvgRating->setText(QString::number(responseObj["averageRating"].toDouble(), 'f', 1));
            m_statTotalIncome->setText("$" + QString::number(responseObj["totalIncome"].toDouble(), 'f', 2));
        }
        else if (type == "action_result") {
            const bool success = responseObj["success"].toBool();
            if (success) {
                requestBooks();
                requestStats();
            } else {
                StyledMessageBox::error(this, "Action failed", responseObj["message"].toString());
            }
        }
        else if (type == "publisher_sales_trend") {
            updateSalesTrend(responseObj["points"].toArray());
        }
        else if (type == "user_info") {
            m_fullName = responseObj["fullName"].toString();
            m_email    = responseObj["email"].toString();
            m_nameLabel->setText(m_fullName.isEmpty() ? m_username : m_fullName);
        }
        else if (type == "profile_update_result") {
            if (responseObj["success"].toBool()) {
                m_fullName = responseObj["fullName"].toString();
                m_email    = responseObj["email"].toString();
                const QString newU = responseObj["username"].toString();
                if (!newU.isEmpty()) {
                    m_username = newU;
                    m_usernameLabel->setText("@" + m_username);
                }
                m_nameLabel->setText(m_fullName.isEmpty() ? m_username : m_fullName);
                if (m_welcomeLabel) m_welcomeLabel->setText(QString("Welcome back, %1!").arg(m_fullName));
                StyledMessageBox::success(this, "Profile", "Profile updated successfully.");
            } else {
                StyledMessageBox::error(this, "Profile", responseObj["message"].toString());
            }
        }
        else if (type == "password_change_result") {
            if (responseObj["success"].toBool())
                StyledMessageBox::success(this, "Password", "Password changed successfully.");
            else
                StyledMessageBox::error(this, "Password", responseObj["message"].toString());
        }
        else if (type == "notifications_list" && responseObj["status"].toString() == "success") {
            m_notifications.clear();
            for (const QJsonValue &v : responseObj["notifications"].toArray()) {
                QJsonObject no = v.toObject();
                Notification n;
                n.id = no["id"].toInt();
                n.userId = m_publisherId;
                n.title = no["title"].toString();
                n.message = no["message"].toString();
                n.date = QDateTime::fromString(no["date"].toString(), Qt::ISODate);
                n.isRead = no["isRead"].toBool();
                m_notifications.push_back(n);
            }
            rebuildNotificationList();
            updateNotificationBadge();
        }
        else if (type == "notification_new") {
            QJsonObject no = responseObj["notification"].toObject();
            Notification n;
            n.id = no["id"].toInt();
            n.userId = m_publisherId;
            n.title = no["title"].toString();
            n.message = no["message"].toString();
            n.date = QDateTime::fromString(no["date"].toString(), Qt::ISODate);
            n.isRead = false;
            m_notifications.prepend(n);
            rebuildNotificationList();
            updateNotificationBadge();
            showNotificationToast(n.title, n.message);
        }
        else if (type == "notification_mark_read_result" && responseObj["status"].toString() == "success") {
            int notifId = responseObj["notificationId"].toInt();
            for (auto &n : m_notifications) if (n.id == notifId) { n.isRead = true; break; }
            rebuildNotificationList();
            updateNotificationBadge();
        }
        else if (type == "notifications_mark_all_read_result" && responseObj["status"].toString() == "success") {
            for (auto &n : m_notifications) n.isRead = true;
            rebuildNotificationList();
            updateNotificationBadge();
        }
        else if (type == "notification_delete_result") {
            if (responseObj["status"].toString() == "success") {
                int notifId = responseObj["notificationId"].toInt();
                for (int i = 0; i < m_notifications.size(); ++i) {
                    if (m_notifications[i].id == notifId) { m_notifications.remove(i); break; }
                }
                rebuildNotificationList();
                updateNotificationBadge();
            } else {
                StyledMessageBox::error(this, "Delete Failed", responseObj["message"].toString());
            }
        }
        else if (type == "notifications_clear_all_result") {
            if (responseObj["status"].toString() == "success") {
                m_notifications.clear();
                rebuildNotificationList();
                updateNotificationBadge();
            } else {
                StyledMessageBox::error(this, "Clear Failed", responseObj["message"].toString());
            }
        }
        else {
            const QString status = responseObj["status"].toString();
            if (status == "success") {
                requestBooks();
                requestStats();
            } else if (status == "error") {
                StyledMessageBox::error(this, "Action failed", responseObj["message"].toString());
            }
        }
    }
}

void PublisherPanel::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    qWarning() << "PublisherPanel socket error:" << m_socket->errorString();
}

void PublisherPanel::switchPage(int index)
{
    m_stackedWidget->setCurrentIndex(index);
    updateButtonStyles(index);
}

void PublisherPanel::updateButtonStyles(int currentIndex)
{
    QString normalStyle =
        "QPushButton { background-color: transparent; border: none; border-radius: 8px; padding: 10px; font-size: 13px; color: #9A8FA0; text-align: left; padding-left: 12px; }"
        "QPushButton:hover { background-color: #1F1724; color: #EAEAEA; }";

    QString activeStyle =
        "QPushButton { background-color: #7C3E66; border: none; border-radius: 8px; padding: 10px; font-size: 13px; font-weight: bold; color: #FFFFFF; text-align: left; padding-left: 12px; }";

    m_btnStats->setStyleSheet(currentIndex == 0 ? activeStyle : normalStyle);
    m_btnBooks->setStyleSheet(currentIndex == 1 ? activeStyle : normalStyle);
    m_btnNotifications->setStyleSheet(currentIndex == 2 ? activeStyle : normalStyle);
}

void PublisherPanel::filterBooks(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        populateBooksGrid(m_allBooks);
        return;
    }

    QVector<Book> filtered;
    for (const Book &b : std::as_const(m_allBooks)) {
        if (b.title.contains(text, Qt::CaseInsensitive)) {
            filtered.push_back(b);
        }
    }
    populateBooksGrid(filtered);
}

static void styleAxis(QAbstractAxis *ax)
{
    ax->setLabelsBrush(QBrush(QColor(kTextDim)));
    ax->setGridLineColor(QColor(kGrid));
    ax->setLinePenColor(QColor(kGrid));
    QFont f = ax->labelsFont(); f.setPointSize(8); ax->setLabelsFont(f);
}

static QChart *makeDarkChart()
{
    auto *chart = new QChart;
    chart->setBackgroundBrush(Qt::transparent);
    chart->legend()->hide();
    chart->setMargins(QMargins(4, 4, 4, 4));
    return chart;
}

void PublisherPanel::replaceChart(QChartView *view, QChart *chart)
{
    QChart *old = view->chart();
    view->setChart(chart);
    delete old;
}

QWidget *PublisherPanel::makeStatCard(const QString &icon, const QString &iconBg,
                                      const QString &title, const QString &subtitle,
                                      QLabel *&valueOut)
{
    auto *card = new QWidget(this);
    card->setStyleSheet(QString("background-color:%1;border:1px solid %2;border-radius:10px;")
                            .arg(kCardBg, kCardBorder));
    auto *h = new QHBoxLayout(card);
    h->setContentsMargins(16, 14, 16, 14);
    h->setSpacing(14);

    auto *iconLabel = new QLabel(icon, card);
    iconLabel->setFixedSize(48, 48);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet(QString("background-color:%1;border:none;border-radius:10px;font-size:22px;")
                                 .arg(iconBg));

    auto *v = new QVBoxLayout;
    v->setSpacing(2);
    auto *titleLabel = new QLabel(title, card);
    titleLabel->setStyleSheet("color:#EAEAEA;font-size:12px;font-weight:bold;border:none;background:transparent;");
    valueOut = new QLabel("—", card);
    valueOut->setStyleSheet("color:#FFFFFF;font-size:24px;font-weight:bold;border:none;background:transparent;");
    auto *subLabel = new QLabel(subtitle, card);
    subLabel->setStyleSheet(QString("color:%1;font-size:10px;border:none;background:transparent;").arg(kTextDim));
    v->addWidget(titleLabel);
    v->addWidget(valueOut);
    v->addWidget(subLabel);

    h->addWidget(iconLabel);
    h->addLayout(v);
    h->addStretch();
    return card;
}

QWidget *PublisherPanel::makeSectionCard(const QString &icon,
                                         const QString &title,
                                         QWidget *content,
                                         QWidget *headerRight)
{
    auto *card = new QWidget(this);

    card->setStyleSheet(
        QString(
            "QWidget{"
            "background-color:%1;"
            "border:1px solid %2;"
            "border-radius:10px;"
            "}"
            ).arg(kCardBg, kCardBorder)
        );


    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(16, 14, 16, 14);
    v->setSpacing(10);


    // Header
    auto *headerWidget = new QWidget(card);
    headerWidget->setStyleSheet("background:transparent;border:none;");

    auto *header = new QHBoxLayout(headerWidget);
    header->setContentsMargins(0,0,0,0);
    header->setSpacing(8);


    // Icon
    auto *iconLabel = new QLabel(headerWidget);

    iconLabel->setPixmap(
        QPixmap(icon).scaled(
            18,
            18,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
            )
        );

    iconLabel->setFixedSize(22,22);
    iconLabel->setAlignment(Qt::AlignCenter);


    // Title
    auto *titleLabel = new QLabel(title, headerWidget);

    titleLabel->setStyleSheet(
        "color:#EAEAEA;"
        "font-size:13px;"
        "font-weight:bold;"
        "border:none;"
        "background:transparent;"
        );


    header->addWidget(iconLabel);
    header->addWidget(titleLabel);
    header->addStretch();


    if (headerRight)
        header->addWidget(headerRight);


    v->addWidget(headerWidget);
    v->addWidget(content);


    return card;
}

QTableWidget *PublisherPanel::makeTopTable()
{
    auto *t = new QTableWidget(5, 3, this);
    t->setHorizontalHeaderLabels({"#", "Book Title", "Sales"});
    t->verticalHeader()->setVisible(false);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionMode(QAbstractItemView::NoSelection);
    t->setFocusPolicy(Qt::NoFocus);
    t->setShowGrid(false);
    t->setFixedHeight(232);
    t->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    t->setColumnWidth(0, 40);
    t->setColumnWidth(2, 70);
    t->setStyleSheet(QString(
                         "QTableWidget{background:transparent;border:none;color:#EAEAEA;font-size:12px;}"
                         "QTableWidget::item{border-bottom:1px solid %1;padding:4px;}"
                         "QHeaderView::section{background-color:#1A141F;color:%2;border:none;padding:6px;font-size:11px;}")
                         .arg(kCardBorder, kTextDim));
    return t;
}

QWidget *PublisherPanel::createStatsPage()
{
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea{border:none;background:transparent;}");

    auto *page = new QWidget;
    page->setStyleSheet("background:transparent;");
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 25, 30, 25);
    layout->setSpacing(18);

    // header
    m_welcomeLabel = new QLabel(QString("Welcome back, %1!")
                                    .arg(m_fullName.isEmpty() ? m_username : m_fullName), page);
    m_welcomeLabel->setStyleSheet("color:#FFFFFF;font-size:22px;font-weight:bold;border:none;");
    auto *sub = new QLabel("Here's an overview of your books and sales performance.", page);
    sub->setStyleSheet(QString("color:%1;font-size:12px;border:none;").arg(kTextDim));
    layout->addWidget(m_welcomeLabel);
    layout->addWidget(sub);

    // stat cards
    auto *cardsRow = new QHBoxLayout;
    cardsRow->setSpacing(16);
    cardsRow->addWidget(makeStatCard("📚", "#8B5CF6", "Total Books",    "Published books", m_statBookCount));
    cardsRow->addWidget(makeStatCard("🛒", "#EC4899", "Total Sales",    "Books sold",      m_statTotalSales));
    cardsRow->addWidget(makeStatCard("💲", "#22C55E", "Total Revenue",  "Total income",    m_statTotalIncome));
    cardsRow->addWidget(makeStatCard("⭐", "#EAB308", "Average Rating", "Out of 5",        m_statAvgRating));
    layout->addLayout(cardsRow);

    // chart views
    auto makeView = [this]() {
        auto *v = new QChartView(makeDarkChart(), this);
        v->setRenderHint(QPainter::Antialiasing);
        v->setStyleSheet("background:transparent;border:none;");
        v->setMinimumHeight(240);
        return v;
    };
    m_trendView  = makeView();
    m_cmpView    = makeView();
    m_ratingView = makeView();
    m_pieView    = makeView();

    m_bestTable  = makeTopTable();
    m_worstTable = makeTopTable();

     //(daily / weekly / monthly per the spec)
    m_trendCombo = new QComboBox(this);
    m_trendCombo->addItems({"Monthly", "Weekly", "Daily"});
    m_trendCombo->setStyleSheet(QString(
                                    "QComboBox{background-color:#1A141F;border:1px solid %1;border-radius:6px;"
                                    "padding:4px 10px;color:#EAEAEA;font-size:11px;}").arg(kCardBorder));
    connect(m_trendCombo, &QComboBox::currentTextChanged, this,
            [this](const QString &) { requestSalesTrend(); });
    auto *pieRow = new QWidget(this);
    pieRow->setStyleSheet("background:transparent;border:none;");
    auto *pieRowLayout = new QHBoxLayout(pieRow);
    pieRowLayout->setContentsMargins(0, 0, 0, 0);
    pieRowLayout->addWidget(m_pieView, 3);
    auto *legendHolder = new QWidget(pieRow);
    legendHolder->setStyleSheet("background:transparent;border:none;");
    m_pieLegendLayout = new QVBoxLayout(legendHolder);
    m_pieLegendLayout->setContentsMargins(0, 20, 0, 20);
    m_pieLegendLayout->setSpacing(8);
    m_pieLegendLayout->addStretch();
    pieRowLayout->addWidget(legendHolder, 2);

    auto *grid = new QGridLayout;
    grid->setSpacing(18);

    grid->addWidget(
        makeSectionCard(
            ":/icons/trophy-solid.png",
            "Top 5 Best Selling Books",
            m_bestTable
            ),
        0, 0
        );

    grid->addWidget(
        makeSectionCard(
            ":/icons/chart-bar-solid.png",
            "Sales Trend",
            m_trendView,
            m_trendCombo
            ),
        0, 1
        );

    grid->addWidget(
        makeSectionCard(
            ":/icons/arrow-trend-down-solid.png",
            "Top 5 Worst Selling Books",
            m_worstTable
            ),
        1, 0
        );

    grid->addWidget(
        makeSectionCard(
            ":/icons/arrow-trend-up-solid.png",
            "Sales Comparison (Top 5 Books)",
            m_cmpView
            ),
        1, 1
        );

    grid->addWidget(
        makeSectionCard(
            ":/icons/star-solid.png",
            "Average Rating per Book",
            m_ratingView
            ),
        2, 0
        );

    grid->addWidget(
        makeSectionCard(
            ":/icons/chart-pie-solid.png",
            "Revenue Share per Book",
            pieRow
            ),
        2, 1
        );

    grid->setColumnStretch(0, 2);
    grid->setColumnStretch(1, 3);

    layout->addLayout(grid);
    scroll->setWidget(page);
    return scroll;
}

// ---------- data → UI ----------

void PublisherPanel::fillTopTable(QTableWidget *table, const QVector<Book> &books)
{
    table->clearContents();
    table->setRowCount(qMin(5, static_cast<int>(books.size())));
    for (int i = 0; i < table->rowCount(); ++i) {
        auto *rank = new QTableWidgetItem(QString::number(i + 1));
        auto *title = new QTableWidgetItem(books[i].title);
        auto *sales = new QTableWidgetItem(QString::number(books[i].totalSales));
        sales->setForeground(QColor(kAccent));
        sales->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->setItem(i, 0, rank);
        table->setItem(i, 1, title);
        table->setItem(i, 2, sales);
    }
}

void PublisherPanel::updateDashboard()
{
    QVector<Book> books;
    for (const Book &b : std::as_const(m_allBooks))
        if (b.status != -1) books.push_back(b);
    if (books.isEmpty()) return;

    // best / worst tables
    QVector<Book> sorted = books;
    std::sort(sorted.begin(), sorted.end(),
              [](const Book &a, const Book &b) { return a.totalSales > b.totalSales; });
    fillTopTable(m_bestTable, sorted);
    QVector<Book> worst = sorted;
    std::reverse(worst.begin(), worst.end());
    fillTopTable(m_worstTable, worst);

    const QVector<Book> top5 = sorted.mid(0, 5);
    auto wrapTitle = [](QString t) { return t.replace(' ', '\n'); };

    {
        auto *set = new QBarSet("Sales");
        set->setColor(QColor(kAccent));
        set->setLabelColor(QColor("#EAEAEA"));
        QStringList cats;
        double maxV = 0;
        for (const Book &b : top5) { *set << b.totalSales; cats << wrapTitle(b.title); maxV = qMax(maxV, double(b.totalSales)); }
        auto *series = new QBarSeries;
        series->append(set);
        series->setLabelsVisible(true);
        series->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd);

        QChart *chart = makeDarkChart();
        chart->addSeries(series);
        auto *ax = new QBarCategoryAxis; ax->append(cats); styleAxis(ax);
        auto *ay = new QValueAxis; ay->setRange(0, maxV * 1.2); ay->setLabelFormat("%d"); styleAxis(ay);
        chart->addAxis(ax, Qt::AlignBottom); chart->addAxis(ay, Qt::AlignLeft);
        series->attachAxis(ax); series->attachAxis(ay);
        replaceChart(m_cmpView, chart);
    }

    {
        auto *set = new QBarSet("Rating");
        set->setColor(QColor(kAccent));
        set->setLabelColor(QColor("#EAEAEA"));
        QStringList cats;
        for (const Book &b : books) { *set << b.averageRating; cats << wrapTitle(b.title); }
        auto *series = new QBarSeries;
        series->append(set);
        series->setLabelsVisible(true);
        series->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd);
        series->setLabelsFormat("@value");

        QChart *chart = makeDarkChart();
        chart->addSeries(series);
        auto *ax = new QBarCategoryAxis; ax->append(cats); styleAxis(ax);
        auto *ay = new QValueAxis; ay->setRange(0, 5.5); ay->setTickCount(6); ay->setLabelFormat("%d"); styleAxis(ay);
        chart->addAxis(ax, Qt::AlignBottom); chart->addAxis(ay, Qt::AlignLeft);
        series->attachAxis(ax); series->attachAxis(ay);
        replaceChart(m_ratingView, chart);
    }

    {
        static const QStringList palette = {"#A85CF0", "#EC4899", "#22C55E", "#EAB308", "#3B82F6"};
        double total = 0;
        QVector<QPair<Book, double>> incomes;
        for (const Book &b : top5) {
            double income = m_incomeByBookId.value(b.id, b.price * b.totalSales); // fallback if server didn't send it
            incomes.push_back({b, income});
            total += income;
        }

        auto *series = new QPieSeries;
        // clear old legend rows
        while (m_pieLegendLayout->count() > 1) {
            QLayoutItem *it = m_pieLegendLayout->takeAt(0);
            if (it->widget()) it->widget()->deleteLater();
            delete it;
        }
        for (int i = 0; i < incomes.size(); ++i) {
            const int pct = total > 0 ? qRound(incomes[i].second / total * 100.0) : 0;
            QPieSlice *slice = series->append(QString::number(pct) + "%", incomes[i].second);
            slice->setColor(QColor(palette[i % palette.size()]));
            slice->setLabelVisible(true);
            slice->setLabelPosition(QPieSlice::LabelInsideHorizontal);
            slice->setLabelColor(Qt::white);
            slice->setBorderWidth(0);

            auto *row = new QLabel(QString(
                                       "<span style='color:%1;font-size:14px;'>●</span> "
                                       "<span style='color:#EAEAEA;font-size:12px;'>%2</span> "
                                       "<span style='color:%3;font-size:12px;'><b>$%4</b> (%5%)</span>")
                                       .arg(palette[i % palette.size()], incomes[i].first.title,
                                            kTextDim, QString::number(incomes[i].second, 'f', 2), QString::number(pct)));
            row->setStyleSheet("border:none;background:transparent;");
            m_pieLegendLayout->insertWidget(m_pieLegendLayout->count() - 1, row);
        }

        QChart *chart = makeDarkChart();
        chart->addSeries(series);
        replaceChart(m_pieView, chart);
    }
}

void PublisherPanel::updateSalesTrend(const QJsonArray &pts)
{
    auto *line = new QLineSeries;
    line->setColor(QColor(kAccent));
    line->setPointsVisible(true);
    QPen pen{QColor(kAccent)};
    pen.setWidth(2);
    line->setPen(pen);

    auto *axX = new QCategoryAxis;
    axX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    double maxV = 0;
    for (int i = 0; i < pts.size(); ++i) {
        const QJsonObject p = pts[i].toObject();
        const double sales = p["sales"].toDouble();
        line->append(i, sales);
        maxV = qMax(maxV, sales);

        QString label = p["period"].toString();               // "2026-07", "2026-W28" or "2026-07-14"
        const QDate d = QDate::fromString(label + "-01", "yyyy-MM-dd");
        if (d.isValid()) label = d.toString("MMM");           // monthly → "Jul"
        axX->append(label, i);
    }
    axX->setRange(-0.2, pts.size() - 0.8);
    styleAxis(axX);
    auto *base = new QLineSeries;
    for (int i = 0; i < pts.size(); ++i) base->append(i, 0);
    auto *area = new QAreaSeries(line, base);
    QLinearGradient g(QPointF(0, 0), QPointF(0, 1));
    g.setCoordinateMode(QGradient::ObjectBoundingMode);
    g.setColorAt(0.0, QColor(168, 92, 240, 140));
    g.setColorAt(1.0, QColor(168, 92, 240, 10));
    area->setBrush(g);
    area->setPen(Qt::NoPen);

    QChart *chart = makeDarkChart();
    chart->addSeries(area);
    chart->addSeries(line);
    auto *axY = new QValueAxis; axY->setRange(0, maxV * 1.15); axY->setLabelFormat("%d"); styleAxis(axY);
    chart->addAxis(axX, Qt::AlignBottom); chart->addAxis(axY, Qt::AlignLeft);
    area->attachAxis(axX); area->attachAxis(axY);
    line->attachAxis(axX); line->attachAxis(axY);
    replaceChart(m_trendView, chart);
}

void PublisherPanel::requestSalesTrend()
{
    QJsonObject req;
    req["action"] = "publisher_get_sales_trend";
    req["publisherId"] = m_publisherId;
    req["granularity"] = m_trendCombo ? m_trendCombo->currentText().toLower() : "monthly";
    sendRequest(req);
}

void PublisherPanel::handleEditProfile()
{
    EditProfileDialog dialog(m_username, m_fullName, m_email, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    // empty field = keep current value
    const QString newUsername = dialog.username().isEmpty() ? m_username : dialog.username();
    const QString newFullName = dialog.fullName().isEmpty() ? m_fullName : dialog.fullName();
    const QString newEmail    = dialog.email().isEmpty()    ? m_email    : dialog.email();
    if (newUsername != m_username || newFullName != m_fullName || newEmail != m_email) {
        QJsonObject req;
        req["action"]      = "user_update_profile";
        req["userId"]      = m_publisherId;        // <-- ID, not username
        req["newUsername"] = newUsername;
        req["fullName"]    = newFullName;
        req["email"]       = newEmail;
        sendRequest(req);
    }
    if (dialog.wantsPasswordChange()) {
        QJsonObject req;
        req["action"]      = "user_change_password";
        req["userId"]      = m_publisherId;        // <-- ID here too
        req["oldPassword"] = dialog.oldPassword();
        req["newPassword"] = dialog.newPassword();
        sendRequest(req);
    }
}

void PublisherPanel::showBlockedOverlay()
{
    if (m_blockOverlay) return;

    this->setEnabled(false);

    m_blockOverlay = new QWidget(this);
    m_blockOverlay->setGeometry(this->rect());
    m_blockOverlay->setStyleSheet("background-color: rgba(6, 5, 8, 220);");
    m_blockOverlay->setEnabled(true);

    QLabel *msgLabel = new QLabel(
        "Your account has been blocked by an administrator.", m_blockOverlay);
    msgLabel->setStyleSheet(
        "color: #EAEAEA; font-size: 18px; font-weight: bold; background: transparent;");
    msgLabel->setAlignment(Qt::AlignCenter);
    msgLabel->setWordWrap(true);

    QVBoxLayout *overlayLayout = new QVBoxLayout(m_blockOverlay);
    overlayLayout->setContentsMargins(60, 0, 60, 0);
    overlayLayout->addWidget(msgLabel);

    m_blockOverlay->raise();
    m_blockOverlay->show();

    QTimer::singleShot(10000, this, []() {
        qApp->quit();
    });
}

void PublisherPanel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_blockOverlay)
        m_blockOverlay->setGeometry(this->rect());
}

#include "userpanel.h"
#include "genreselectiondialog.h"
#include "pdfreaderdialog.h"
#include <QPdfDocument>
#include <QEventLoop>
#include <QMessageBox>
#include <QPainter>
#include <QFrame>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <algorithm>
#include "src/publisherPanel/editprofiledialog.h"
#include "mylibrarypage.h"
#include "shelfcard.h"
#include "styledmessagebox.h"

static const char *kCardBg     = "#120E14";
static const char *kCardBorder = "#1F1724";
static const char *kAccent     = "#7C3E66";
static const char *kTextDim    = "#9A8FA0";

static QPixmap makeCoverPixmap(const Book &b, const QSize &size)
{
    if (!b.coverImagePath.isEmpty()) {
        QFileInfo info(b.coverImagePath);
        // Cache path on user client side
        QString localCachePath = QCoreApplication::applicationDirPath() + "/cache/covers/" + info.fileName();
        QString errorMsg;

        if (downloadFileFromServer(b.coverImagePath, localCachePath, errorMsg)) {
            QPixmap img(localCachePath);
            if (!img.isNull()) {
                return img.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)
                .copy(0, 0, size.width(), size.height());
            }
        }
    }

    QPixmap canvas(size);
    static const QStringList colors = {"#3B2A4D", "#4D2A3E", "#2A3E4D", "#2A4D3B", "#4D3B2A"};
    canvas.fill(QColor(colors[qAbs(b.id) % colors.size()]));
    QPainter p(&canvas);
    p.setPen(QColor("#EAEAEA"));
    QFont f = p.font(); f.setBold(true); f.setPointSize(9); p.setFont(f);
    p.drawText(canvas.rect().adjusted(8, 8, -8, -8),
               Qt::AlignCenter | Qt::TextWordWrap, b.title);
    p.end();
    return canvas;
}

UserPanel::UserPanel(int userId, const QString &fullName, const QString &username, QWidget *parent)
    : QWidget(parent), m_userId(userId), m_fullName(fullName), m_username(username)
{
    m_socket = new QTcpSocket(this);
    setupUi();

    m_startupLoader = new QWidget(this);
    m_startupLoader->setGeometry(this->rect());
    m_startupLoader->setStyleSheet("background-color: #060508;");

    QVBoxLayout *loaderLayout = new QVBoxLayout(m_startupLoader);
    loaderLayout->setAlignment(Qt::AlignCenter);
    loaderLayout->setSpacing(12);

    QLabel *loaderIcon = new QLabel("📚", m_startupLoader);
    loaderIcon->setStyleSheet("font-size:36px;background:transparent;");
    loaderIcon->setAlignment(Qt::AlignCenter);

    m_loaderStatusLabel = new QLabel("Connecting...", m_startupLoader);
    m_loaderStatusLabel->setStyleSheet("color:#EAEAEA;font-size:13px;font-weight:bold;background:transparent;");
    m_loaderStatusLabel->setAlignment(Qt::AlignCenter);

    m_loaderProgressBar = new QProgressBar(m_startupLoader);
    m_loaderProgressBar->setFixedWidth(240);
    m_loaderProgressBar->setRange(0, 0); // indeterminate until we know how many covers to load
    m_loaderProgressBar->setTextVisible(false);
    m_loaderProgressBar->setStyleSheet(
        "QProgressBar{background-color:#1A141F;border:1px solid #1F1724;border-radius:6px;height:8px;}"
        "QProgressBar::chunk{background-color:#7C3E66;border-radius:6px;}");

    loaderLayout->addWidget(loaderIcon);
    loaderLayout->addWidget(m_loaderStatusLabel);
    loaderLayout->addWidget(m_loaderProgressBar);

    m_startupLoader->raise();
    m_startupLoader->show();

    // Safety net — if a cover download stalls/fails silently, don't leave the
    // overlay stuck forever. Doesn't affect the progress logic if it completes normally.
    QTimer::singleShot(4000, this, [this]() {
        if (m_startupLoaderActive) updateStartupProgress(QString(), m_coversExpected, m_coversExpected);
    });

    connect(m_socket, &QTcpSocket::readyRead, this, &UserPanel::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &UserPanel::onSocketError);
    connect(m_socket, &QTcpSocket::connected, this, [this]() {
        QJsonObject subReq;
        subReq["action"] = "user_subscribe";
        subReq["userId"] = m_userId;
        sendRequest(subReq);

        requestNotifications();

        requestAllBooks();
        m_cartPage->refreshCart();
        m_wishlistPage->refreshWishlist();

        // Fetch User profile info & favorite genres
        QJsonObject reqUser;
        reqUser["action"] = "user_fetch";
        reqUser["username"] = m_username;
        sendRequest(reqUser);

        QJsonObject reqGenres;
        reqGenres["action"] = "user_get_favorite_genres";
        reqGenres["userId"] = m_userId;
        sendRequest(reqGenres);
        QJsonObject reqOwned;
        reqOwned["action"] = "books_fetch_owned";
        reqOwned["userId"] = m_userId;
        sendRequest(reqOwned);

        QJsonObject reqShelves;
        reqShelves["action"] = "shelves_fetch";
        reqShelves["userId"] = m_userId;
        sendRequest(reqShelves);

        QJsonObject reqProgress;
        reqProgress["action"] = "progress_fetch_all";
        reqProgress["userId"] = m_userId;
        sendRequest(reqProgress);
    });

    m_socket->connectToHost("127.0.0.1", 1234);
    switchPage(0);
}

void UserPanel::sendRequest(const QJsonObject &requestObj)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        StyledMessageBox::error(this, "Not connected", "Not connected to the server.");
        return;
    }
    m_socket->write(QJsonDocument(requestObj).toJson(QJsonDocument::Compact) + "\n");
}

void UserPanel::requestAllBooks()
{
    QJsonObject req;
    req["action"] = "books_fetch_all";
    req["activeOnly"] = true;
    sendRequest(req);
}

void UserPanel::setupUi()
{
    setStyleSheet("background-color: #060508; color: #EAEAEA; font-family: 'Segoe UI', Arial;");
    this->resize(1000, 700);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Sidebar Container
    QWidget *sidebar = new QWidget(this);
    sidebar->setFixedWidth(220);
    sidebar->setStyleSheet("background-color: #120E14; border-right: 1px solid #1F1724;");

    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(15, 25, 15, 25);
    sidebarLayout->setSpacing(12);

    QWidget *avatarRow = new QWidget(sidebar);
    avatarRow->setStyleSheet("background: transparent; border: none;");
    QHBoxLayout *avatarRowLayout = new QHBoxLayout(avatarRow);
    avatarRowLayout->setContentsMargins(0, 0, 0, 0);
    avatarRowLayout->setSpacing(8);

    QLabel *avatarLabel = new QLabel(avatarRow);

    QPixmap avatar(":/icons/avater.png");
    avatarLabel->setPixmap(avatar.scaled(
        64, 64,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation));

    avatarLabel->setFixedSize(64, 64);
    avatarLabel->setStyleSheet("border: none; background: transparent;");
    avatarLabel->setAlignment(Qt::AlignCenter);

    QPushButton *editProfileBtn = new QPushButton(avatarRow);

    editProfileBtn->setIcon(QIcon(":/icons/pen.png"));
    editProfileBtn->setIconSize(QSize(18, 18));
    editProfileBtn->setFixedSize(30, 30);
    editProfileBtn->setCursor(Qt::PointingHandCursor);
    editProfileBtn->setToolTip("Edit profile");
    editProfileBtn->setStyleSheet(
        "QPushButton { background-color: #1F1724; border: 1px solid #2A2233; border-radius: 8px; font-size: 13px; }"
        "QPushButton:hover { background-color: #7C3E66; border-color: #B06B96; }");
    connect(editProfileBtn, &QPushButton::clicked, this, &UserPanel::handleEditProfile);

    avatarRowLayout->addStretch();
    avatarRowLayout->addWidget(avatarLabel);
    avatarRowLayout->addWidget(editProfileBtn);
    avatarRowLayout->addStretch();

    QLabel *roleLabel = new QLabel("Reader", sidebar);
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
    sidebarLayout->addSpacing(10);

    // --- Favorite genres: shows what the user picked, with an edit affordance ---
    QWidget *genresRow = new QWidget(sidebar);
    genresRow->setStyleSheet("background: transparent; border: none;");
    QHBoxLayout *genresRowLayout = new QHBoxLayout(genresRow);
    genresRowLayout->setContentsMargins(0, 0, 0, 0);
    genresRowLayout->setSpacing(6);

    m_genresLabel = new QLabel(genresRow);
    m_genresLabel->setStyleSheet("color: #9A8FA0; font-size: 11px; background: transparent; border: none;");
    m_genresLabel->setAlignment(Qt::AlignCenter);
    m_genresLabel->setWordWrap(true);
    m_genresLabel->setText("Favorite genres: not set yet");

    m_editGenresBtn = new QPushButton(genresRow);
    m_editGenresBtn->setIcon(QIcon(":/icons/pen.png"));
    m_editGenresBtn->setIconSize(QSize(12, 12));
    m_editGenresBtn->setFixedSize(20, 20);
    m_editGenresBtn->setCursor(Qt::PointingHandCursor);
    m_editGenresBtn->setToolTip("Edit favorite genres");
    m_editGenresBtn->setStyleSheet(
        "QPushButton { background-color: #1F1724; border: 1px solid #2A2233; border-radius: 6px; }"
        "QPushButton:hover { background-color: #7C3E66; border-color: #B06B96; }");
    connect(m_editGenresBtn, &QPushButton::clicked, this, &UserPanel::handleEditGenres);

    genresRowLayout->addStretch();
    genresRowLayout->addWidget(m_genresLabel);
    genresRowLayout->addWidget(m_editGenresBtn);
    genresRowLayout->addStretch();

    sidebarLayout->addWidget(genresRow);
    sidebarLayout->addSpacing(15);

    m_btnHome = new QPushButton("Home Discovery", sidebar);
    m_btnHome->setIcon(QIcon(":/icons/home.png"));
    m_btnHome->setIconSize(QSize(20, 20));

    QString menuBtnStyle =
        "QPushButton { background-color: transparent; border: none; border-radius: 8px; padding: 10px; font-size: 13px; color: #9A8FA0; text-align: left; padding-left: 12px; }"
        "QPushButton:hover { background-color: #1F1724; color: #EAEAEA; }";
    m_btnHome->setStyleSheet(menuBtnStyle);
    m_btnHome->setCursor(Qt::PointingHandCursor);

    m_btnLibrary = new QPushButton("My Library", sidebar);
    m_btnLibrary->setIcon(QIcon(":/icons/book.png"));
    m_btnLibrary->setIconSize(QSize(20,20));

    m_btnLibrary->setStyleSheet(menuBtnStyle);
    m_btnLibrary->setCursor(Qt::PointingHandCursor);

    m_btnCart = new QPushButton("Shopping Cart", sidebar);
    m_btnCart->setIcon(QIcon(":/icons/cart.png"));
    m_btnCart->setIconSize(QSize(20,20));

    m_btnCart->setStyleSheet(menuBtnStyle);
    m_btnCart->setCursor(Qt::PointingHandCursor);

    auto *cartBtnLayout = new QGridLayout(m_btnCart);
    cartBtnLayout->setContentsMargins(0, 3, 6, 0);
    cartBtnLayout->setColumnStretch(0, 1);
    cartBtnLayout->setRowStretch(1, 1);

    m_cartBadge = new QLabel(m_btnCart);
    m_cartBadge->setAlignment(Qt::AlignCenter);
    m_cartBadge->setFixedHeight(18);
    m_cartBadge->setMinimumWidth(18);
    m_cartBadge->setStyleSheet(
        "QLabel { background-color: #e46060; color: white; font-size: 10px; font-weight: 700;"
        " border-radius: 9px; padding: 0 4px; }");
    m_cartBadge->hide();

    cartBtnLayout->addWidget(m_cartBadge, 0, 1, Qt::AlignTop | Qt::AlignRight);

    m_btnWishlist = new QPushButton("Wishlist", sidebar);
    m_btnWishlist->setIcon(QIcon(":/icons/heart.png"));
    m_btnWishlist->setIconSize(QSize(20,20));

    m_btnWishlist->setStyleSheet(menuBtnStyle);
    m_btnWishlist->setCursor(Qt::PointingHandCursor);

    connect(m_btnHome, &QPushButton::clicked, this, [this]() { switchPage(0); });
    sidebarLayout->addWidget(m_btnHome);
    connect(m_btnLibrary, &QPushButton::clicked, this, [this]() { switchPage(5); });
    sidebarLayout->addWidget(m_btnLibrary);
    connect(m_btnCart, &QPushButton::clicked, this, [this]() { switchPage(1); });
    sidebarLayout->addWidget(m_btnCart);
    connect(m_btnWishlist, &QPushButton::clicked, this, [this]() { switchPage(3); });
    sidebarLayout->addWidget(m_btnWishlist);

    m_btnNotifications = new QPushButton("Notifications", sidebar);
    m_btnNotifications->setIcon(QIcon(":/icons/bell.png"));
    m_btnNotifications->setIconSize(QSize(20,20));

    m_btnNotifications->setStyleSheet(menuBtnStyle);
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

    connect(m_btnNotifications, &QPushButton::clicked, this, [this]() { switchPage(4); });
    sidebarLayout->addWidget(m_btnNotifications);

    sidebarLayout->addStretch();

    m_btnLogout = new QPushButton("Logout", sidebar);
    m_btnLogout->setIcon(QIcon(":/icons/logout.png"));
    m_btnLogout->setIconSize(QSize(20,20));

    m_btnLogout->setCursor(Qt::PointingHandCursor);
    m_btnLogout->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #7C3E66; border-radius: 8px; padding: 8px; font-weight: bold; color: #D9C2D1; text-align: left; padding-left: 12px; }"
        "QPushButton:hover { background-color: rgba(124, 62, 102, 60); color: white; border: 1px solid #B06B96; }"
        );
    connect(m_btnLogout, &QPushButton::clicked, this, &UserPanel::logoutRequested);
    sidebarLayout->addWidget(m_btnLogout);

    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->addWidget(createHomePage());

    m_cartPage = new ShoppingCartPage(m_socket, m_userId, this);
    connect(m_cartPage, &ShoppingCartPage::checkoutCompleted, this, [this](int purchaseId) {
        Q_UNUSED(purchaseId);

        QJsonObject reqOwned;
        reqOwned["action"] = "books_fetch_owned";
        reqOwned["userId"] = m_userId;
        sendRequest(reqOwned);
    });
    connect(m_cartPage, &ShoppingCartPage::cartUpdated, this, &UserPanel::updateHero);
    connect(m_cartPage, &ShoppingCartPage::cartUpdated, this, &UserPanel::updateCartBadge);
    m_stackedWidget->addWidget(m_cartPage);

    m_detailsPage = new BookDetailsPage(this);
    m_stackedWidget->addWidget(m_detailsPage);

    connect(m_detailsPage, &BookDetailsPage::backRequested, this, [this] { switchPage(0); });
    connect(m_detailsPage, &BookDetailsPage::addToCartRequested, this, &UserPanel::addToCart);
    connect(m_detailsPage, &BookDetailsPage::wishlistToggleRequested, this,[this](int id) {
        if(m_ownedBookIds.contains(id))
            return;
        QJsonObject req;
        req["action"] = m_wishlistPage->containsBook(id) ? "wishlist_remove" : "wishlist_add";
        req["userId"] = m_userId;
        req["bookId"] = id;
        sendRequest(req);
    });
    connect(m_detailsPage, &BookDetailsPage::openBookRequested, this,[this](int id) {
        openBookReader(id);
    });
    connect(m_detailsPage, &BookDetailsPage::reviewSubmitted, this,[this](int id, int rating, const QString &text) {
        if (rating <= 0) {
            StyledMessageBox::error(this, "Rating Required",
                                    "Please select a star rating before submitting your review.");
            return;
        }
        QJsonObject req;
        req["action"] = "submit_review";
        req["userId"] = m_userId;
        req["bookId"] = id;
        req["rating"] = rating;
        req["comment"] = text;
        sendRequest(req);
    });

    m_wishlistPage = new WishlistPage(m_socket, m_userId, this);
    connect(m_wishlistPage, &WishlistPage::addToCartRequested, this, &UserPanel::addToCart);
    connect(m_wishlistPage, &WishlistPage::bookDetailsRequested, this, &UserPanel::openBookDetails);
    connect(m_wishlistPage, &WishlistPage::exploreBooksRequested, this, [this] { switchPage(0); });
    connect(m_wishlistPage, &WishlistPage::wishlistUpdated, this, [this] {
        updateHero();
        if (m_stackedWidget->currentWidget() == m_detailsPage)
            m_detailsPage->setWishlisted(m_wishlistPage->containsBook(m_detailsPage->currentBookId()));
    });
    m_stackedWidget->addWidget(m_wishlistPage);

    m_notifPage = createNotificationsPage();
    m_stackedWidget->addWidget(m_notifPage);

    setupMyLibraryPage();
    m_stackedWidget->addWidget(m_libraryPage);

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(m_stackedWidget);
}

void UserPanel::setupMyLibraryPage()
{
    m_libraryPage = new MyLibraryPage(this);

    m_libraryPage->setCoverProvider([](const Book &b, const QSize &size) {
        return makeCoverPixmap(b, size);
    });

    connect(m_libraryPage, &MyLibraryPage::bookOpenRequested, this, &UserPanel::openBookDetails);

    connect(m_libraryPage, &MyLibraryPage::bookReadRequested, this, [this](int bookId) {
        openBookReader(bookId);
    });

    connect(m_libraryPage, &MyLibraryPage::moveBookToShelfRequested, this, [this](int bookId, int shelfId) {
        QJsonObject req;
        req["action"] = "shelf_add_book";
        req["shelfId"] = shelfId;
        req["bookId"] = bookId;
        sendRequest(req);
    });

    connect(m_libraryPage, &MyLibraryPage::createShelfRequested, this,
            [this](const QString &name, const QString &description, const QColor &color) {
                Q_UNUSED(description);
                Q_UNUSED(color);
                QJsonObject req;
                req["action"] = "shelf_create";
                req["userId"] = m_userId;
                req["title"] = name;
                sendRequest(req);
            });

    connect(m_libraryPage, &MyLibraryPage::shelfOpened, this, [this](int shelfId) {
        m_openingShelfId = shelfId;
        QJsonObject req;
        req["action"] = "shelf_fetch_books";
        req["shelfId"] = shelfId;
        sendRequest(req);
    });

    connect(m_libraryPage, &MyLibraryPage::bookFavoriteToggleRequested, this, [this](int bookId) {
        if (m_favoriteBookIds.contains(bookId)) {
            QJsonObject req;
            req["action"] = "shelf_remove_book";
            req["shelfId"] = m_favoritesShelfId;
            req["bookId"] = bookId;
            sendRequest(req);
            return;
        }
        if (m_favoritesShelfId == -1) {
            m_pendingFavoriteBookId = bookId;
            QJsonObject req;
            req["action"] = "shelf_create";
            req["userId"] = m_userId;
            req["title"] = "Favorites";
            sendRequest(req);
            return;
        }
        QJsonObject req;
        req["action"] = "shelf_add_book";
        req["shelfId"] = m_favoritesShelfId;
        req["bookId"] = bookId;
        sendRequest(req);
    });

    connect(m_libraryPage, &MyLibraryPage::editShelfRequested, this,
            [this](int shelfId, const QString &newName, const QString &newDescription, const QColor &newColor) {
                Q_UNUSED(newDescription);
                Q_UNUSED(newColor);
                QJsonObject req;
                req["action"] = "shelf_update";
                req["shelfId"] = shelfId;
                req["title"] = newName;
                sendRequest(req);
            });

    connect(m_libraryPage, &MyLibraryPage::deleteShelfRequested, this, [this](int shelfId) {
        QJsonObject req;
        req["action"] = "shelf_delete";
        req["shelfId"] = shelfId;
        sendRequest(req);
    });

    connect(m_libraryPage, &MyLibraryPage::removeBookFromShelfRequested, this, [this](int bookId, int shelfId) {
        QJsonObject req;
        req["action"] = "shelf_remove_book";
        req["shelfId"] = shelfId;
        req["bookId"] = bookId;
        sendRequest(req);
    });
}

Book UserPanel::enrichedBook(int id, const QString &fallbackTitle, const QString &fallbackAuthor) const
{
    for (const Book &b : m_storeBooks) {
        if (b.id == id) return b;
    }
    Book b;
    b.id = id;
    b.title = fallbackTitle;
    b.author = fallbackAuthor;
    return b;
}

void UserPanel::finalizeShelfSummaries()
{
    QVector<ShelfSummary> summaries;
    m_favoritesShelfId = -1;
    m_favoriteBookIds.clear();

    for (const Shelf &s : m_shelves) {
        ShelfSummary summary;
        summary.shelf = s;
        const QVector<Book> books = m_shelfBooksCache.value(s.id);
        summary.bookCount = books.size();
        summary.previewBooks = books.mid(0, 4);
        summaries.push_back(summary);

        if (s.title.compare("Favorites", Qt::CaseInsensitive) == 0) {
            m_favoritesShelfId = s.id;
            for (const Book &b : books) m_favoriteBookIds.insert(b.id);
        }
    }

    if (m_libraryPage) {
        m_libraryPage->setShelves(summaries);
        m_libraryPage->setFavoriteBookIds(m_favoriteBookIds);
        m_libraryPage->setStatistics(m_ownedBooksFull.size(), m_shelves.size(),
                                     m_readingProgressByBookId.size(), m_favoriteBookIds.size());
        rebuildContinueReadingItems();
    }
}

void UserPanel::switchPage(int index)
{
    m_stackedWidget->setCurrentIndex(index);
    QString normalStyle =
        "QPushButton { background-color: transparent; border: none; border-radius: 8px; padding: 10px; font-size: 13px; color: #9A8FA0; text-align: left; padding-left: 12px; }"
        "QPushButton:hover { background-color: #1F1724; color: #EAEAEA; }";
    QString activeStyle =
        "QPushButton { background-color: #7C3E66; border: none; border-radius: 8px; padding: 10px; font-size: 13px; font-weight: bold; color: #FFFFFF; text-align: left; padding-left: 12px; }";

    m_btnHome->setStyleSheet(index == 0 ? activeStyle : normalStyle);
    m_btnCart->setStyleSheet(index == 1 ? activeStyle : normalStyle);
    m_btnWishlist->setStyleSheet(index == 3 ? activeStyle : normalStyle);
    m_btnLibrary->setStyleSheet(index == 5 ? activeStyle : normalStyle);

    if (index == 1) {
        m_cartPage->refreshCart();
    }
    else if (index == 3) {
        m_wishlistPage->refreshWishlist();
    }
    else if (index == 5) {
        QJsonObject reqOwned;
        reqOwned["action"] = "books_fetch_owned";
        reqOwned["userId"] = m_userId;
        sendRequest(reqOwned);

        QJsonObject reqShelves;
        reqShelves["action"] = "shelves_fetch";
        reqShelves["userId"] = m_userId;
        sendRequest(reqShelves);

        QJsonObject reqProgress;                    // add this block
        reqProgress["action"] = "progress_fetch_all";
        reqProgress["userId"] = m_userId;
        sendRequest(reqProgress);
    }
}

QWidget *UserPanel::createHomePage()
{
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea{border:none;background:#060508;}");  // main bg

    auto *page = new QWidget;
    page->setStyleSheet("background:#060508;");  // match main
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 20, 30, 30);
    layout->setSpacing(24);

    // ---------- search bar (subtle glow on focus) ----------
    m_searchEdit = new QLineEdit(page);
    m_searchEdit->setPlaceholderText("Search by title, author, or publisher...");

    QAction *searchAction = new QAction(QIcon(":/icons/magnifying-glass-solid.png"), "", m_searchEdit);
    m_searchEdit->addAction(searchAction, QLineEdit::LeadingPosition);
    m_searchEdit->setFixedHeight(44);
    m_searchEdit->setStyleSheet(QString(
                                    "QLineEdit{background-color:%1;border:1px solid %2;border-radius:22px;"
                                    "padding-left:20px;padding-right:20px;color:#EAEAEA;font-size:14px;}"
                                    "QLineEdit:focus{border:1px solid %3;background-color:#1A141F;}")
                                    .arg(kCardBg, kCardBorder, kAccent));
    layout->addWidget(m_searchEdit);

    // ---- search results panel (hidden until user types) ----
    m_searchResultsPanel = new QWidget(page);
    m_searchResultsPanel->setStyleSheet("background:transparent;border:none;");

    auto *srLayout = new QVBoxLayout(m_searchResultsPanel);
    srLayout->setContentsMargins(0, 0, 0, 0);
    srLayout->setSpacing(12);

    auto *resultsHeaderRow = new QWidget(m_searchResultsPanel);
    resultsHeaderRow->setStyleSheet("background:transparent;border:none;");
    auto *resultsHeaderLayout = new QHBoxLayout(resultsHeaderRow);
    resultsHeaderLayout->setContentsMargins(0, 0, 0, 0);
    resultsHeaderLayout->setSpacing(10);

    m_searchResultsLabel = new QLabel(resultsHeaderRow);
    m_searchResultsLabel->setStyleSheet("color:#FFFFFF;font-size:16px;font-weight:bold;border:none;background:transparent;");

    m_searchResultsCloseBtn = new QPushButton("✕ Close", resultsHeaderRow);
    m_searchResultsCloseBtn->setCursor(Qt::PointingHandCursor);
    m_searchResultsCloseBtn->setStyleSheet(
        "QPushButton{background:transparent;border:1px solid #3A3244;border-radius:8px;"
        "padding:6px 14px;color:#9A8FA0;font-size:12px;}"
        "QPushButton:hover{border-color:#7C3E66;color:#EAEAEA;background-color:#1A141F;}");
    connect(m_searchResultsCloseBtn, &QPushButton::clicked, this, &UserPanel::closeResultsView);

    resultsHeaderLayout->addWidget(m_searchResultsLabel);
    resultsHeaderLayout->addStretch();
    resultsHeaderLayout->addWidget(m_searchResultsCloseBtn);
    srLayout->addWidget(resultsHeaderRow);

    m_searchResultsGrid = new QGridLayout;
    m_searchResultsGrid->setSpacing(16);
    srLayout->addLayout(m_searchResultsGrid);
    srLayout->addStretch();

    m_searchResultsPanel->hide();
    layout->addWidget(m_searchResultsPanel);

    // ---- home sections wrapper ----
    m_homeSections = new QWidget(page);
    m_homeSections->setStyleSheet("background:transparent;border:none;");
    auto *homeLayout = new QVBoxLayout(m_homeSections);
    homeLayout->setContentsMargins(0, 0, 0, 0);
    homeLayout->setSpacing(24);
    layout->addWidget(m_homeSections);

    // ---------- hero banner (unchanged, already good) ----------
    auto *hero = new QWidget(page);
    hero->setFixedHeight(300);
    hero->setStyleSheet(QString("background-color:%1;border:1px solid %2;border-radius:14px;")
                            .arg(kCardBg, kCardBorder));
    auto *heroLayout = new QHBoxLayout(hero);
    heroLayout->setContentsMargins(24, 24, 24, 24);
    heroLayout->setSpacing(28);

    auto makeArrow = [hero](const QString &t) {
        auto *b = new QPushButton(t, hero);
        b->setFixedSize(40, 40);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(
            "QPushButton {background-color: rgba(31, 23, 36, 0.85);border: 1px solid #4A3F55;"
            "border-radius: 20px;color: #EAEAEA;font-size: 18px;font-weight: bold;}"
            "QPushButton:hover {background-color: #7C3E66;border-color: #B06B96;color: #FFFFFF;}"
            "QPushButton:pressed {background-color: #4D2640;border-color: #7C3E66;}");
        return b;
    };
    auto *prevBtn = makeArrow("←");
    auto *nextBtn = makeArrow("→");

    m_heroCover = new QLabel(hero);
    m_heroCover->setFixedSize(180, 260);
    m_heroCover->setScaledContents(true);
    m_heroCover->setStyleSheet("border:none;border-radius:10px;");
    hero->setStyleSheet(R"(
        QWidget {
            border-radius:14px;
            border:1px solid #2A2230;

            background-image:url(:/heroback.png);
            background-position:center;
            background-repeat:no-repeat;
        }
        )");

    auto *heroInfo = new QVBoxLayout;
    heroInfo->setSpacing(6);
    m_heroGenre = new QLabel(hero);
    m_heroGenre->setStyleSheet(QString(
                                   "background-color:%1;color:white;border:none;border-radius:12px;"
                                   "padding:5px 14px;font-size:11px;font-weight:bold;").arg(kAccent));
    m_heroGenre->setFixedHeight(26);
    auto *genreWrap = new QHBoxLayout; genreWrap->addWidget(m_heroGenre); genreWrap->addStretch();

    m_heroTitle = new QLabel(hero);
    m_heroTitle->setStyleSheet("color:#FFFFFF;font-size:26px;font-weight:bold;border:none;background:transparent;");
    m_heroTitle->setWordWrap(true);
    m_heroAuthor = new QLabel(hero);
    m_heroAuthor->setStyleSheet(QString("color:%1;font-size:15px;border:none;background:transparent;").arg(kTextDim));
    m_heroRating = new QLabel(hero);
    m_heroRating->setStyleSheet("color:#EAB308;font-size:14px;border:none;background:transparent;");
    m_heroDesc = new QLabel(hero);
    m_heroDesc->setStyleSheet(QString("color:%1;font-size:13px;line-height:1.4;border:none;background:transparent;").arg(kTextDim));
    m_heroDesc->setWordWrap(true);
    m_heroDesc->setMaximumHeight(50);
    auto *heroBtns = new QHBoxLayout;
    heroBtns->setSpacing(10);
    auto *viewBtn = new QPushButton("View Details", hero);
    viewBtn->setStyleSheet(
        "QPushButton{background:rgba(26, 20, 31, 0.75); border:2px solid #4A3F55; border-radius:8px;"
        "padding:9px 19px; color:#EAEAEA; font-size:13px; font-weight:bold;}"
        "QPushButton:hover{border-color:#7C3E66; background-color:#7C3E66; color:#FFFFFF;}"
        "QPushButton:pressed{background-color:#4D2640; border-color:#4D2640;}");
    // 1. Create the buttons ONCE (no parent needed if adding to layout, or pass hero)
    m_heroCartBtn = new QPushButton("Add to Cart", hero);
    m_heroCartBtn->setStyleSheet(QString(
        "QPushButton {background-color: %1;border: 1px solid #5A2D4A;border-radius: 8px;"
        "padding: 10px 20px;color: white;font-size: 13px;font-weight: bold;}"
        "QPushButton:hover {background-color: #B06B96;border-color: #7C3E66;}"
        "QPushButton:pressed {background-color: #4D2640;border-color: #4D2640;}").arg(kAccent));

    m_heroOpenBtn = new QPushButton("Open Book", hero);
    m_heroOpenBtn->setStyleSheet(
        "QPushButton {background-color: #3FAE72;border: 1px solid #2C7A50;border-radius: 8px;"
        "padding: 10px 20px;color: white;font-size: 13px;font-weight: bold;}"
        "QPushButton:hover {background-color: #55C687;border-color: #3FAE72;}"
        "QPushButton:pressed {background-color: #2C7A50;}");
    m_heroOpenBtn->hide(); // Hide by default

    m_heroWishlistBtn = new QPushButton("♡", hero);
    m_heroWishlistBtn->setFixedSize(40, 40);
    m_heroWishlistBtn->setStyleSheet(
        "QPushButton {background: rgba(26, 20, 31, 0.75);border: 2px solid #4A3F55;border-radius: 20px;"
        "color: #FF6B9D;font-size: 18px;font-weight: bold;}"
        "QPushButton:hover {border-color: #FF6B9D;background-color: rgba(255, 107, 157, 40);}"
        "QPushButton:pressed {background-color: rgba(255, 107, 157, 80);}");

    viewBtn->setCursor(Qt::PointingHandCursor);
    m_heroCartBtn->setCursor(Qt::PointingHandCursor);
    m_heroWishlistBtn->setCursor(Qt::PointingHandCursor);
    heroBtns->addWidget(viewBtn);
    heroBtns->addWidget(m_heroCartBtn);
    heroBtns->addWidget(m_heroOpenBtn);
    heroBtns->addWidget(m_heroWishlistBtn);
    heroBtns->addStretch();

    connect(viewBtn, &QPushButton::clicked, this, [this] {
        if (!m_heroBooks.isEmpty()) openBookDetails(m_heroBooks[m_heroIndex].id);
    });
    connect(m_heroCartBtn, &QPushButton::clicked, this, [this] {
        if (!m_heroBooks.isEmpty()) {
            int id = m_heroBooks[m_heroIndex].id;
            if (m_cartPage && m_cartPage->containsBook(id)) {
                switchPage(1);
            } else {
                addToCart(id);
            }
        }
    });
    connect(m_heroWishlistBtn, &QPushButton::clicked, this, [this] {
        if (!m_heroBooks.isEmpty()) toggleWishlist(m_heroBooks[m_heroIndex].id);
    });
    connect(prevBtn, &QPushButton::clicked, this, [this] {
        if (m_heroBooks.isEmpty()) return;
        m_heroIndex = (m_heroIndex - 1 + m_heroBooks.size()) % m_heroBooks.size();
        updateHero();
    });
    connect(nextBtn, &QPushButton::clicked, this, [this] {
        if (m_heroBooks.isEmpty()) return;
        m_heroIndex = (m_heroIndex + 1) % m_heroBooks.size();
        updateHero();
    });
    connect(m_heroOpenBtn, &QPushButton::clicked, this, [this] {
        if (!m_heroBooks.isEmpty())
            openBookReader(m_heroBooks[m_heroIndex].id);
    });

    heroInfo->addLayout(genreWrap);
    heroInfo->addWidget(m_heroTitle);
    heroInfo->addWidget(m_heroAuthor);
    heroInfo->addWidget(m_heroRating);
    heroInfo->addWidget(m_heroDesc);
    heroInfo->addSpacing(6);
    heroInfo->addLayout(heroBtns);
    heroInfo->addStretch();

    heroLayout->addWidget(prevBtn);
    heroLayout->addWidget(m_heroCover);
    heroLayout->addLayout(heroInfo, 1);
    heroLayout->addWidget(nextBtn);
    layout->addWidget(hero);

    // ---------- categories (horizontal scroll) ----------
    struct Cat { const char *img; const char *name; };
    static const Cat cats[] = {
        {":/genres/scifi.jpg",      "Science Fiction"},
        {":/genres/fantasy.jpg",    "Fantasy"},
        {":/genres/mystery.jpg",    "Mystery"},
        {":/genres/romance.jpg",    "Romance"},
        {":/genres/horror.jpg",     "Horror"},
        {":/genres/history.jpg",    "History"},
        {":/genres/technology.jpg", "Technology"},
        {":/genres/business.jpg",   "Business"},
        {":/genres/psychology.jpg", "Psychology"},
        {":/genres/art.jpg",        "Art & Design"},
        {":/genres/cooking.jpg",    "Cooking"},
        {":/genres/health.jpg",     "Health & Sport"},
        {":/genres/children.jpg",   "Children"},
        {":/genres/poetry.jpg",     "Poetry"},
        {":/genres/travel.jpg",     "Travel"},
        {":/genres/biography.jpg",  "Biography"}
    };

    auto *catContainer = new QWidget(page);
    catContainer->setStyleSheet("background:transparent;border:none;");
    auto *catVLayout = new QVBoxLayout(catContainer);
    catVLayout->setContentsMargins(0, 0, 0, 0);
    catVLayout->setSpacing(10);
    auto *catTitle = new QLabel("Categories", catContainer);
    catTitle->setStyleSheet("color:#FFFFFF;font-size:16px;font-weight:bold;border:none;background:transparent;");
    catVLayout->addWidget(catTitle);

    auto *catScroll = new QScrollArea(catContainer);
    catScroll->setWidgetResizable(false);
    catScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    catScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    catScroll->setFrameShape(QFrame::NoFrame);
    catScroll->setFixedHeight(130);
    catScroll->setStyleSheet(
        "QScrollArea{border:none;background:transparent;}"
        "QScrollBar:horizontal{height:8px;background:#1A141F;border-radius:4px;}"
        "QScrollBar::handle:horizontal{background:#7C3E66;border-radius:4px;min-width:40px;}"
        "QScrollBar::handle:horizontal:hover{background:#B06B96;}");

    auto *catScrollContent = new QWidget;
    catScrollContent->setStyleSheet("background:transparent;border:none;");
    auto *catRow = new QHBoxLayout(catScrollContent);
    catRow->setContentsMargins(0, 0, 0, 0);
    catRow->setSpacing(14);

    for (const auto &c : cats) {
        auto *btn = new QPushButton(catScrollContent);
        btn->setFixedSize(120, 110);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setIconSize(QSize(120, 110));
        btn->setStyleSheet(
            "QPushButton{border:1px solid #1F1724;border-radius:12px;background:transparent;padding:0;}"
            "QPushButton:hover{border-color:#7C3E66;transform:scale(1.02);}");

        QPixmap px(c.img);
        QPixmap canvas(120, 110);
        canvas.fill(QColor("#1A141F"));
        QPainter p(&canvas);
        if (!px.isNull())
            p.drawPixmap(0, 0, px.scaled(canvas.size(), Qt::KeepAspectRatioByExpanding,
                                         Qt::SmoothTransformation));
        QLinearGradient g(0, 55, 0, 110);
        g.setColorAt(0, QColor(0,0,0,0)); g.setColorAt(1, QColor(0,0,0,220));
        p.fillRect(canvas.rect(), g);
        p.setPen(Qt::white);
        QFont f = p.font(); f.setPointSize(9); f.setBold(true); p.setFont(f);
        p.drawText(canvas.rect().adjusted(6,0,-6,-8), Qt::AlignBottom|Qt::AlignHCenter, c.name);
        p.end();
        btn->setIcon(QIcon(canvas));

        const QString genreName = c.name;
        connect(btn, &QPushButton::clicked, this, [this, genreName] { openGenre(genreName); });
        catRow->addWidget(btn);
    }
    catRow->addStretch();
    catScroll->setWidget(catScrollContent);
    catVLayout->addWidget(catScroll);
    layout->addWidget(catContainer);

    // ---------- horizontal scrolling book rows ----------
    layout->addWidget(makeHorizontalScrollRow("Recommended for You", m_rowRecommended,
                                              [this] { return m_recommendedBooks; }));
    layout->addWidget(makeHorizontalScrollRow("New Releases", m_rowNewReleases,
                                              [this] { return m_newestBooks; }));
    layout->addWidget(makeHorizontalScrollRow("Bestsellers", m_rowBestsellers,
                                              [this] { return m_bestsellerBooks; }));
    layout->addWidget(makeHorizontalScrollRow("Free Books", m_rowFree,
                                              [this] { return m_freeBooks; }));

    layout->addStretch();
    scroll->setWidget(page);
    setupSearch();
    return scroll;
}

QWidget *UserPanel::makeBookRow(const QString &title, QHBoxLayout *&rowLayoutOut)
{
    auto *card = new QWidget(this);
    card->setStyleSheet(QString("background-color:%1;border:1px solid %2;border-radius:14px;")
                            .arg(kCardBg, kCardBorder));
    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(18, 14, 18, 18);
    auto *head = new QLabel(title, card);
    head->setStyleSheet("color:white;font-size:15px;font-weight:bold;border:none;background:transparent;");
    v->addWidget(head);
    rowLayoutOut = new QHBoxLayout;
    rowLayoutOut->setSpacing(14);
    rowLayoutOut->addStretch();
    v->addLayout(rowLayoutOut);
    return card;
}

QWidget *UserPanel::makeBookCard(const Book &b)
{
    auto *card = new QPushButton();

    card->setFixedSize(128, 240);
    card->setCursor(Qt::PointingHandCursor);

    card->setStyleSheet(
        "QPushButton{background:#1A141F;"
        "border:1px solid #1F1724;"
        "border-radius:10px;}"
        "QPushButton:hover{border-color:#7C3E66;}");

    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(8, 8, 8, 8);
    v->setSpacing(4);

    auto *coverStack = new QWidget(card);
    coverStack->setFixedSize(112, 150);
    auto *coverGrid = new QGridLayout(coverStack);
    coverGrid->setContentsMargins(0, 0, 0, 0);

    auto *cover = new QLabel(coverStack);
    cover->setFixedSize(112, 150);
    cover->setPixmap(makeCoverPixmap(b, QSize(112, 150)));
    cover->setScaledContents(true);
    coverGrid->addWidget(cover, 0, 0);

    if (m_startupLoaderActive && !m_coversLoadedIds.contains(b.id)) {
        m_coversLoadedIds.insert(b.id);
        m_coversLoaded++;
        updateStartupProgress(QString("Loading... (%1/%2)").arg(m_coversLoaded).arg(m_coversExpected),
                              m_coversLoaded, m_coversExpected);
    }

    auto *heartBtn = new QPushButton(coverStack);
    heartBtn->setFixedSize(22, 22);
    heartBtn->setCursor(Qt::PointingHandCursor);
    bool isOwned = m_ownedBookIds.contains(b.id);
    bool inWishlist = m_wishlistPage && m_wishlistPage->containsBook(b.id);
    heartBtn->setText(inWishlist ? "♥" : "♡");
    heartBtn->setStyleSheet(
        "QPushButton{background-color:rgba(0,0,0,150);border:none;border-radius:11px;"
        "color:#FF6B9D;font-size:13px;}"
        "QPushButton:hover{background-color:rgba(0,0,0,210);}");
    heartBtn->setVisible(!isOwned);
    coverGrid->addWidget(heartBtn, 0, 0, Qt::AlignTop | Qt::AlignRight);
    connect(heartBtn, &QPushButton::clicked, this, [this, id = b.id, heartBtn] {
        toggleWishlist(id);
        bool nowIn = heartBtn->text() == QString("♡");
        heartBtn->setText(nowIn ? "♥" : "♡");
    });

    auto *title = new QLabel(b.title, card);
    title->setWordWrap(true);

    auto *author = new QLabel(b.author, card);

    v->addWidget(coverStack);
    v->addWidget(title);
    v->addWidget(author);
    v->addStretch();

    connect(card, &QPushButton::clicked, this,
            [this, id = b.id]() {
                openBookDetails(id);
            });

    return card;
}

void UserPanel::fillBookRow(QHBoxLayout *rowLayout,
                            const QVector<Book> &books,
                            int maxCount)
{
    while (QLayoutItem *item = rowLayout->takeAt(0))
    {
        if (item->widget())
            delete item->widget();

        delete item;
    }

    int shown = 0;

    for (const Book &b : books)
    {
        if (maxCount > 0 && shown >= maxCount)
            break;

        QWidget *card = makeBookCard(b);

        qDebug() << "Adding:" << b.title;

        rowLayout->addWidget(card);
        shown++;
    }

    rowLayout->addStretch();

    rowLayout->invalidate();
    rowLayout->activate();

    if (rowLayout->parentWidget())
    {
        rowLayout->parentWidget()->adjustSize();
        rowLayout->parentWidget()->update();
    }

    qDebug() << "Cards shown =" << shown;
}

void UserPanel::rebuildHomeSections()
{
    QVector<Book> byRating = m_storeBooks;
    std::sort(byRating.begin(), byRating.end(),
              [](const Book &a, const Book &b) { return a.averageRating > b.averageRating; });
    m_heroBooks = byRating.mid(0, 4);
    m_heroIndex = 0;
    updateHero();

    QVector<Book> recommended;
    for (const Book &b : std::as_const(byRating))
        if (m_favoriteGenres.contains(b.genre, Qt::CaseInsensitive))
            recommended.push_back(b);
    if (recommended.isEmpty()) recommended = byRating;
    m_recommendedBooks = recommended;
    fillBookRow(m_rowRecommended, m_recommendedBooks);

    QVector<Book> newest = m_storeBooks;
    std::sort(newest.begin(), newest.end(),
              [](const Book &a, const Book &b) { return a.id > b.id; });
    m_newestBooks = newest;
    fillBookRow(m_rowNewReleases, m_newestBooks);

    QVector<Book> best = m_storeBooks;
    std::sort(best.begin(), best.end(),
              [](const Book &a, const Book &b) { return a.totalSales > b.totalSales; });
    m_bestsellerBooks = best;
    fillBookRow(m_rowBestsellers, m_bestsellerBooks);

    QVector<Book> free;
    for (const Book &b : std::as_const(m_storeBooks))
        if (b.price <= 0.0) free.push_back(b);
    m_freeBooks = free;
    fillBookRow(m_rowFree, m_freeBooks);
}

void UserPanel::onReadyRead()
{
    while (m_socket->canReadLine()) {
        QByteArray data = m_socket->readLine().trimmed();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError) continue;
        if (!doc.isObject()) continue;

        QJsonObject responseObj = doc.object();
        const QString action = responseObj["action"].toString();
        QString type = responseObj["type"].toString();

        if (action == "notify_account_blocked") {
            showBlockedOverlay();
            return;
        }

        if (responseObj.contains("action")) {
            m_cartPage->handleServerResponse(responseObj);
            m_wishlistPage->handleServerResponse(responseObj);
        }

        if (action == "books_fetch_all_response" && responseObj["status"].toString() == "success") {
            m_storeBooks.clear();
            for (const QJsonValue &val : responseObj["books"].toArray()) {
                const QJsonObject bo = val.toObject();
                Book b;
                b.id = bo["id"].toInt();
                b.title = bo["title"].toString();
                b.author = bo["author"].toString();
                b.genre = bo["genre"].toString();
                b.price = bo["price"].toDouble();
                b.description = bo["description"].toString();
                b.coverImagePath = bo["coverImagePath"].toString();
                b.averageRating = bo["averageRating"].toDouble();
                b.totalSales = bo["totalSales"].toInt();
                b.publisherName = bo["publisherName"].toString();
                qDebug() << b.title << "-> publisher:" << b.publisherName;
                m_storeBooks.push_back(b);
                QJsonObject pubReq;
                pubReq["action"] = "get_publisher_by_book";
                pubReq["bookId"] = b.id;
                sendRequest(pubReq);
            }
            m_coversExpected = m_storeBooks.size();
            m_coversLoaded = 0;
            m_coversLoadedIds.clear();

            if (m_coversExpected == 0)
                updateStartupProgress(QString(), 0, 0); // nothing to load, close immediately
            else
                updateStartupProgress(QString("Loading %1 covers...").arg(m_coversExpected), 0, m_coversExpected);
            rebuildHomeSections();
            m_wishlistPage->setCatalog(m_storeBooks);
        }
        else if (action == "notify_book_updated") {
            int bookId = responseObj["bookId"].toInt();
            int status = responseObj.contains("status") ? responseObj["status"].toInt() : 1;

            if (status != 1) {
                m_storeBooks.removeIf([bookId](const Book &b) { return b.id == bookId; });
            }
            else {
                bool found = false;
                for (Book &b : m_storeBooks) {
                    if (b.id == bookId) {
                        if (responseObj.contains("title")) b.title = responseObj["title"].toString();
                        if (responseObj.contains("author")) b.author = responseObj["author"].toString();
                        if (responseObj.contains("genre")) b.genre = responseObj["genre"].toString();
                        if (responseObj.contains("price")) b.price = responseObj["price"].toDouble();
                        if (responseObj.contains("coverImagePath")) b.coverImagePath = responseObj["coverImagePath"].toString();
                        b.status = status;
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    Book b;
                    b.id = bookId;
                    b.title = responseObj["title"].toString();
                    b.author = responseObj["author"].toString();
                    b.genre = responseObj["genre"].toString();
                    b.price = responseObj["price"].toDouble();
                    b.coverImagePath = responseObj["coverImagePath"].toString();
                    b.status = status;
                    m_storeBooks.push_back(b);
                }
            }

            rebuildHomeSections();
            m_wishlistPage->setCatalog(m_storeBooks);
        }
        else if (action == "notify_book_removed") {
            int bookId = responseObj["bookId"].toInt();

            m_storeBooks.removeIf([bookId](const Book &b) { return b.id == bookId; });

            rebuildHomeSections();
            m_wishlistPage->setCatalog(m_storeBooks);
        }
        else if (type == "favorite_genres" && responseObj["success"].toBool()) {
            m_favoriteGenres.clear();
            for (const QJsonValue &v : responseObj["genres"].toArray()) {
                m_favoriteGenres << v.toString();
            }

            if (m_favoriteGenres.isEmpty()) {
                GenreSelectionDialog dialog({}, this); // first run — nothing pre-selected yet
                if (dialog.exec() == QDialog::Accepted) {
                    m_favoriteGenres = dialog.selectedGenres();
                    saveFavoriteGenres();
                }
            }

            updateGenresLabel();
            rebuildHomeSections();
        }
        else if (type == "favorite_genres_saved" && responseObj["success"].toBool()) {
            updateGenresLabel();
            rebuildHomeSections();
        }
        else if (type == "user_info" && responseObj["status"].toString() == "success") {
            m_fullName = responseObj["fullName"].toString();
            m_email = responseObj["email"].toString();
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

                StyledMessageBox::success(this, "Profile", "Profile updated successfully.");
            } else {
                StyledMessageBox::error(this, "Profile", responseObj["message"].toString());
            }
        }
        else if (type == "password_change_result") {
            if (responseObj["success"].toBool()) {
                StyledMessageBox::success(this, "Password", "Password changed successfully.");
            } else {
                StyledMessageBox::error(this, "Password", responseObj["message"].toString());
            }
        }
        else if (action == "book_reviews_response" && responseObj["status"].toString() == "success") {
            QVector<Review> reviews;
            for (const QJsonValue &val : responseObj["data"].toArray()) {
                QJsonObject ro = val.toObject();
                if (!ro["isApproved"].toBool()) continue;
                Review r;
                r.id = ro["id"].toInt();
                r.username = ro["username"].toString();
                r.rating = ro["rating"].toInt();
                r.comment = ro["comment"].toString();
                r.date = QDateTime::fromString(ro["date"].toString(), Qt::ISODate);
                reviews.push_back(r);
            }
            m_detailsPage->showReviews(reviews);
        }
        else if (action == "submit_review_response") {
            if (responseObj["status"].toString() == "success") {
                StyledMessageBox::success(this, "Review Submitted",
                                          "Thanks! Your review has been submitted and is awaiting admin approval.");
                m_detailsPage->clearReviewForm();
            } else {
                StyledMessageBox::error(this, "Submission Failed", responseObj["message"].toString());
            }
        }
        else if (type == "notifications_list" && responseObj["status"].toString() == "success") {
            m_notifications.clear();
            for (const QJsonValue &v : responseObj["notifications"].toArray()) {
                QJsonObject no = v.toObject();
                Notification n;
                n.id = no["id"].toInt();
                n.userId = m_userId;
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
            n.userId = m_userId;
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
        else if (action == "books_fetch_owned_response" || action == "books_fetch_owned") {
            if (responseObj["status"].toString() == "success") {
                m_ownedBookIds.clear();
                m_ownedBooksFull.clear();
                m_ownedBookPdfPaths.clear();
                QJsonArray booksArr = responseObj["books"].toArray();
                for (const QJsonValue &val : booksArr) {
                    const QJsonObject bo = val.toObject();
                    const int id = bo["id"].toInt();
                    m_ownedBookIds.insert(id);
                    m_ownedBooksFull.push_back(enrichedBook(id, bo["title"].toString(), bo["author"].toString()));
                    m_ownedBookPdfPaths.insert(id,bo["pdfPath"].toString());
                }

                rebuildHomeSections();

                if (m_detailsPage && m_stackedWidget->currentWidget() == m_detailsPage) {
                    m_detailsPage->setOwned(m_ownedBookIds.contains(m_detailsPage->currentBookId()));
                }

                if (m_libraryPage) {
                    m_libraryPage->setMyBooks(m_ownedBooksFull);
                    m_libraryPage->setStatistics(m_ownedBooksFull.size(), m_shelves.size(),
                                                 m_readingProgressByBookId.size(), m_favoriteBookIds.size());
                }
            }
        }
        else if (action == "shelves_fetch_response") {
            if (responseObj["status"].toString() == "success") {
                m_shelves.clear();
                m_shelfBooksCache.clear();
                for (const QJsonValue &val : responseObj["shelves"].toArray()) {
                    QJsonObject so = val.toObject();
                    Shelf s;
                    s.id = so["id"].toInt();
                    s.title = so["title"].toString();
                    m_shelves.push_back(s);
                }

                if (m_shelves.isEmpty()) {
                    finalizeShelfSummaries();
                } else {
                    for (const Shelf &s : m_shelves) {
                        QJsonObject req;
                        req["action"] = "shelf_fetch_books";
                        req["shelfId"] = s.id;
                        sendRequest(req);
                    }
                }
            }
            else if (action == "get_book_page_count_response") {
                int bookId = responseObj["bookId"].toInt();
                m_pendingPageCountRequests.remove(bookId);
                m_totalPagesByBookId[bookId] = responseObj["status"].toString() == "success"
                                                   ? responseObj["pageCount"].toInt() : 0;
                rebuildContinueReadingItems();
            }
        }
        else if (action == "shelf_fetch_books_response") {
            if (responseObj["status"].toString() == "success") {
                int shelfId = responseObj["shelfId"].toInt();
                QVector<Book> books;
                for (const QJsonValue &val : responseObj["books"].toArray()) {
                    QJsonObject bo = val.toObject();
                    int id = bo["id"].toInt();
                    books.push_back(enrichedBook(id, bo["title"].toString(), bo["author"].toString()));
                }
                m_shelfBooksCache[shelfId] = books;

                if (m_openingShelfId == shelfId) {
                    ShelfSummary summary;
                    for (const Shelf &s : m_shelves) {
                        if (s.id == shelfId) { summary.shelf = s; break; }
                    }
                    summary.bookCount = books.size();
                    summary.previewBooks = books.mid(0, 4);
                    if (m_libraryPage) m_libraryPage->showShelfDetail(summary, books);
                    m_openingShelfId = -1;
                }

                if (m_shelfBooksCache.size() >= m_shelves.size()) {
                    finalizeShelfSummaries();
                }
            }
        }
        else if (action == "shelf_create_response") {
            if (responseObj["status"].toString() == "success") {
                Shelf s;
                s.id = responseObj["shelfId"].toInt();
                s.title = responseObj.contains("title") ? responseObj["title"].toString() : QString();
                m_shelves.push_back(s);

                if (m_pendingFavoriteBookId != -1) {
                    m_favoritesShelfId = s.id;
                    QJsonObject req;
                    req["action"] = "shelf_add_book";
                    req["shelfId"] = s.id;
                    req["bookId"] = m_pendingFavoriteBookId;
                    sendRequest(req);
                    m_pendingFavoriteBookId = -1;
                }

                QJsonObject reqShelves;
                reqShelves["action"] = "shelves_fetch";
                reqShelves["userId"] = m_userId;
                sendRequest(reqShelves);
            } else {
                m_pendingFavoriteBookId = -1;
                StyledMessageBox::error(this, "Shelf", responseObj["message"].toString());
            }
        }
        else if (action == "shelf_add_book_response") {
            if (responseObj["status"].toString() == "success") {
                QJsonObject reqShelves;
                reqShelves["action"] = "shelves_fetch";
                reqShelves["userId"] = m_userId;
                sendRequest(reqShelves);
            } else {
                StyledMessageBox::error(this, "Shelf", responseObj["message"].toString());
            }
        }
        else if (action == "progress_fetch_response" || (action.isEmpty() && responseObj.contains("lastPage"))) {
            const int bookId = responseObj.contains("bookId") ? responseObj["bookId"].toInt() : m_pendingReaderBookId;
            if (bookId != m_pendingReaderBookId)
                continue;
            m_pendingReaderBookId = -1;
            const int lastPage = responseObj["status"].toString() == "success"
                                     ? responseObj["lastPage"].toInt(0) : 0;
            launchPdfReader(bookId, m_pendingReaderLocalPath, m_pendingReaderTitle, lastPage);
        }
        else if (action == "progress_update_response") {
            if (responseObj["status"].toString() != "success") {
                qWarning() << "Failed to save reading progress:" << responseObj["message"].toString();
            }
        }else if (action == "shelf_remove_book_response") {
            if (responseObj["status"].toString() == "success") {
                int shelfId = responseObj["shelfId"].toInt();
                int bookId = responseObj["bookId"].toInt();
                if (shelfId == m_favoritesShelfId)
                    m_favoriteBookIds.remove(bookId);

                QJsonObject reqShelves;
                reqShelves["action"] = "shelves_fetch";
                reqShelves["userId"] = m_userId;
                sendRequest(reqShelves);
                if (m_libraryPage && m_libraryPage->currentlyViewedShelfId() == shelfId) {
                    m_openingShelfId = shelfId;
                    QJsonObject req;
                    req["action"] = "shelf_fetch_books";
                    req["shelfId"] = shelfId;
                    sendRequest(req);
                }
            } else {
                StyledMessageBox::error(this, "Shelf", responseObj["message"].toString());
            }
        }
        else if (action == "shelf_update_response") {
            if (responseObj["status"].toString() == "success") {
                QJsonObject reqShelves;
                reqShelves["action"] = "shelves_fetch";
                reqShelves["userId"] = m_userId;
                sendRequest(reqShelves);
            } else {
                StyledMessageBox::error(this, "Shelf", responseObj["message"].toString());
            }
        }
        else if (action == "shelf_delete_response") {
            if (responseObj["status"].toString() == "success") {
                int shelfId = responseObj["shelfId"].toInt();
                m_shelfBooksCache.remove(shelfId);
                if (m_libraryPage && m_libraryPage->currentlyViewedShelfId() == shelfId)
                    m_libraryPage->backToLibrary();

                QJsonObject reqShelves;
                reqShelves["action"] = "shelves_fetch";
                reqShelves["userId"] = m_userId;
                sendRequest(reqShelves);
            } else {
                StyledMessageBox::error(this, "Shelf", responseObj["message"].toString());
            }
        }
        else if (action == "progress_fetch_all_response") {
            if (responseObj["status"].toString() == "success") {
                for (const QJsonValue &val : responseObj["progress"].toArray()) {
                    QJsonObject po = val.toObject();
                    m_readingProgressByBookId[po["bookId"].toInt()] = po["lastPage"].toInt();
                }
                rebuildContinueReadingItems();
            }
        }
        else if (action == "get_book_page_count_response") {
            int bookId = responseObj["bookId"].toInt();
            m_pendingPageCountRequests.remove(bookId);
            m_totalPagesByBookId[bookId] = responseObj["status"].toString() == "success"
                                               ? responseObj["pageCount"].toInt() : 0;
            rebuildContinueReadingItems();
        }
    }

}

void UserPanel::onSocketError()
{
    qWarning() << "UserPanel socket error: " << m_socket->errorString();
}

void UserPanel::showBlockedOverlay()
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

void UserPanel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_blockOverlay)
        m_blockOverlay->setGeometry(this->rect());
}

QWidget *UserPanel::makeHorizontalScrollRow(const QString &title, QHBoxLayout *&rowLayoutOut,
                                            std::function<QVector<Book>()> getFullList)
{
    auto *container = new QWidget(this);
    container->setStyleSheet("background:transparent;border:none;");
    auto *vLayout = new QVBoxLayout(container);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(10);

    // Header with title + "See All" link
    auto *headerLayout = new QHBoxLayout;
    auto *titleLabel = new QLabel(title, container);
    titleLabel->setStyleSheet("color:#FFFFFF;font-size:16px;font-weight:bold;border:none;background:transparent;");
    auto *seeAllBtn = new QPushButton("See All →", container);
    seeAllBtn->setFlat(true);
    seeAllBtn->setCursor(Qt::PointingHandCursor);
    seeAllBtn->setStyleSheet(
        "QPushButton{color:#7C3E66;font-size:12px;border:none;background:transparent;padding:4px 8px;}"
        "QPushButton:hover{color:#B06B96;text-decoration:underline;}");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(seeAllBtn);
    vLayout->addLayout(headerLayout);

    // Horizontal scroll area (compact preview row)
    auto *scrollArea = new QScrollArea(container);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setFixedHeight(260);
    scrollArea->setStyleSheet(
        "QScrollArea{border:none;background:transparent;}"
        "QScrollBar:horizontal{height:8px;background:#1A141F;border-radius:4px;margin:0;}"
        "QScrollBar::handle:horizontal{background:#7C3E66;border-radius:4px;min-width:40px;}"
        "QScrollBar::handle:horizontal:hover{background:#B06B96;}"
        "QScrollBar::add-line:horizontal,QScrollBar::sub-line:horizontal{width:0px;}"
        "QScrollBar::add-page:horizontal,QScrollBar::sub-page:horizontal{background:none;}");
    auto *scrollContent = new QWidget;
    scrollContent->setStyleSheet("background:transparent;border:none;");
    rowLayoutOut = new QHBoxLayout(scrollContent);
    rowLayoutOut->setContentsMargins(10, 10, 10, 10);
    rowLayoutOut->setSpacing(16);
    rowLayoutOut->setAlignment(Qt::AlignLeft);
    scrollContent->setMinimumHeight(250);
    scrollArea->setWidget(scrollContent);
    vLayout->addWidget(scrollArea);

    // ---- inline "expanded" grid, appears directly under this row ----
    auto *expandPanel = new QWidget(container);
    expandPanel->setStyleSheet("background:transparent;border:none;");
    auto *expandLayout = new QVBoxLayout(expandPanel);
    expandLayout->setContentsMargins(0, 6, 0, 0);
    auto *expandGrid = new QGridLayout;
    expandGrid->setSpacing(16);
    expandLayout->addLayout(expandGrid);
    expandPanel->hide();
    vLayout->addWidget(expandPanel);

    auto expanded = std::make_shared<bool>(false);

    connect(seeAllBtn, &QPushButton::clicked, this,
            [this, seeAllBtn, scrollArea, expandPanel, expandGrid, getFullList, expanded]() {
                *expanded = !*expanded;
                if (*expanded) {
                    while (QLayoutItem *it = expandGrid->takeAt(0)) {
                        if (it->widget()) it->widget()->deleteLater();
                        delete it;
                    }
                    const QVector<Book> books = getFullList();
                    const int columns = 5;
                    int row = 0, col = 0;
                    for (const Book &b : books) {
                        expandGrid->addWidget(makeBookCard(b), row, col);
                        if (++col >= columns) { col = 0; ++row; }
                    }
                    scrollArea->hide();
                    expandPanel->show();
                    seeAllBtn->setText("Show Less ↑");
                } else {
                    expandPanel->hide();
                    scrollArea->show();
                    seeAllBtn->setText("See All →");
                }
            });

    return container;
}

void UserPanel::updateHero()
{
    if (m_heroBooks.isEmpty()) return;

    const Book &b = m_heroBooks[m_heroIndex];
    m_heroCover->setPixmap(makeCoverPixmap(b, QSize(180, 260)));
    m_heroGenre->setText(b.genre.isEmpty() ? "Featured" : b.genre);
    m_heroTitle->setText(b.title);
    m_heroAuthor->setText(b.author);
    m_heroRating->setText(QString("⭐ %1").arg(QString::number(b.averageRating, 'f', 1)));
    m_heroDesc->setText(b.description);

    const bool isOwned = m_ownedBookIds.contains(b.id);

    if (m_cartPage && m_cartPage->containsBook(b.id)) {
        m_heroCartBtn->setText("✓ In Cart");
        m_heroCartBtn->setStyleSheet(
            "QPushButton {background-color: #2A4D3B;border: 1px solid #1E382A;border-radius: 8px;"
            "padding: 10px 20px;color: white;font-size: 13px;font-weight: bold;}"
            "QPushButton:hover {background-color: #38664E;border-color: #2A4D3B;}"
            "QPushButton:pressed {background-color: #1E382A;}");
    } else {
        m_heroCartBtn->setText("Add to Cart");
        m_heroCartBtn->setStyleSheet(QString(
            "QPushButton {background-color: %1;border: 1px solid #5A2D4A;border-radius: 8px;"
            "padding: 10px 20px;color: white;font-size: 13px;font-weight: bold;}"
            "QPushButton:hover {background-color: #B06B96;border-color: #7C3E66;}"
            "QPushButton:pressed {background-color: #4D2640;border-color: #4D2640;}").arg(kAccent));
    }
    if (m_heroWishlistBtn) {
        bool inWishlist = m_wishlistPage && m_wishlistPage->containsBook(b.id);
        m_heroWishlistBtn->setText(inWishlist ? "♥" : "♡");
        m_heroWishlistBtn->setStyleSheet(QString(
            "QPushButton{background:%1; border:2px solid %2; border-radius:20px;"
            "color:#FF6B9D; font-size:16px; font-weight:bold;}"
            "QPushButton:hover{border-color:#FF6B9D; background-color:rgba(255,107,157,60);}")
            .arg(inWishlist ? "rgba(255,107,157,40)" : "rgba(26, 20, 31, 0.75)",
            inWishlist ? "#FF6B9D" : "#4A3F55"));
        m_heroWishlistBtn->setVisible(!isOwned);
    }

    m_heroCartBtn->setVisible(!isOwned);
    m_heroOpenBtn->setVisible(isOwned);
}

void UserPanel::updateCartBadge()
{
    const int count = m_cartPage ? m_cartPage->itemCount() : 0;

    if (count <= 0) {
        m_cartBadge->hide();
        return;
    }

    m_cartBadge->setText(count > 99 ? QStringLiteral("99+") : QString::number(count));
    m_cartBadge->show();
}

void UserPanel::requestNotifications()
{
    QJsonObject req;
    req["action"] = "notifications_fetch";
    req["userId"] = m_userId;
    sendRequest(req);
}

QWidget *UserPanel::createNotificationsPage()
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
    auto *subtitle = new QLabel("What's new with your books and orders", page);
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
        req["userId"] = m_userId;
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
        req["userId"] = m_userId;
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
        req["userId"] = m_userId;
        req["notificationId"] = notifId;
        sendRequest(req);
    });
    layout->addWidget(m_notifListWidget, 1);

    return page;
}

void UserPanel::rebuildNotificationList()
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

        // Left accent bar — the strongest unread cue in the panel.
        auto *accentBar = new QLabel(row);
        accentBar->setFixedWidth(3);
        accentBar->setMinimumHeight(1);
        accentBar->setStyleSheet(QString("background-color:%1;border-radius:2px;")
                                     .arg(n.isRead ? "transparent" : visual.accent));

        // Category icon bubble.
        auto *iconBubble = new QLabel(visual.icon, row);
        iconBubble->setFixedSize(36, 36);
        iconBubble->setAlignment(Qt::AlignCenter);
        iconBubble->setStyleSheet(QString(
                                      "font-size:16px;border-radius:18px;background-color:%1;border:none;")
                                      .arg(visual.accent + "26")); // soft tint of the accent color

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
            req["userId"] = m_userId;
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

void UserPanel::updateNotificationBadge()
{
    int unread = 0;
    for (const Notification &n : m_notifications) if (!n.isRead) unread++;

    if (unread <= 0) { m_notifBadge->hide(); return; }
    m_notifBadge->setText(unread > 99 ? QStringLiteral("99+") : QString::number(unread));
    m_notifBadge->show();
}

void UserPanel::showNotificationToast(const QString &title, const QString &message)
{
    NotificationToast::show(this, title, message);
}

void UserPanel::setupSearch()
{
    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(300);   // debounce: search 300ms after typing stops

    connect(m_searchEdit, &QLineEdit::textChanged, this, [this] {
        m_searchTimer->start();
    });
    connect(m_searchTimer, &QTimer::timeout, this, [this] {
        runSearch(m_searchEdit->text());
    });
    // Enter searches immediately
    connect(m_searchEdit, &QLineEdit::returnPressed, this, [this] {
        m_searchTimer->stop();
        runSearch(m_searchEdit->text());
    });
}

void UserPanel::runSearch(const QString &text)
{
    const QString q = text.trimmed();
    if (q.isEmpty()) {
        m_searchResultsPanel->hide();
        m_homeSections->show();
        return;
    }

    QVector<Book> results;
    for (const Book &b : std::as_const(m_storeBooks)) {
        if (b.title.contains(q, Qt::CaseInsensitive) ||
            b.author.contains(q, Qt::CaseInsensitive) ||
            b.publisherName.contains(q, Qt::CaseInsensitive)) {
            results.push_back(b);
        }
    }

    showResultsGrid(
        results.isEmpty() ? QString("No results for \"%1\"").arg(q)
                          : QString("Results for \"%1\" (%2 found)").arg(q).arg(results.size()),
        results);
}


void UserPanel::addToCart(int bookId)       {
    QJsonObject req;
    req["action"] = "add_to_cart";
    req["userId"] = m_userId;
    req["bookId"] = bookId;
    req["quantity"] = 1;

    sendRequest(req);

    StyledMessageBox::success(this, "Added to Cart", "Book successfully added to your cart!");
}

void UserPanel::openBookDetails(int bookId)
{
    for (const Book &b : std::as_const(m_storeBooks)) {
        if (b.id == bookId) {

            m_detailsPage->setBook(b);

            m_detailsPage->setOwned(m_ownedBookIds.contains(bookId));

            m_detailsPage->setWishlisted(m_wishlistPage && m_wishlistPage->containsBook(bookId));

            switchPage(2);

            QJsonObject req;
            req["action"] = "get_book_reviews";
            req["bookId"] = bookId;
            sendRequest(req);

            return;
        }
    }
}

void UserPanel::openGenre(const QString &g)
{
    QVector<Book> results;
    for (const Book &b : std::as_const(m_storeBooks))
        if (b.genre.compare(g, Qt::CaseInsensitive) == 0)
            results.push_back(b);

    showFullList(g, results);
}

void UserPanel::toggleWishlist(int bookId)
{
    if (!m_wishlistPage) return;

    if(m_ownedBookIds.contains(bookId))
        return;

    QJsonObject req;
    req["action"] = m_wishlistPage->containsBook(bookId) ? "wishlist_remove" : "wishlist_add";
    req["userId"] = m_userId;
    req["bookId"] = bookId;
    sendRequest(req);
}

void UserPanel::showResultsGrid(const QString &headerText, const QVector<Book> &results)
{
    while (QLayoutItem *it = m_searchResultsGrid->takeAt(0)) {
        if (it->widget()) it->widget()->deleteLater();
        delete it;
    }

    m_searchResultsLabel->setText(headerText);

    const int columns = 5;
    int row = 0, col = 0;
    for (const Book &b : std::as_const(results)) {
        m_searchResultsGrid->addWidget(makeBookCard(b), row, col);
        if (++col >= columns) { col = 0; ++row; }
    }

    m_homeSections->hide();
    m_searchResultsPanel->show();
}

void UserPanel::closeResultsView()
{
    if (m_searchTimer) m_searchTimer->stop();
    if (m_searchEdit) {
        m_searchEdit->blockSignals(true);
        m_searchEdit->clear();
        m_searchEdit->blockSignals(false);
    }
    m_searchResultsPanel->hide();
    m_homeSections->show();
}

void UserPanel::showFullList(const QString &title, const QVector<Book> &books)
{
    if (m_searchTimer) m_searchTimer->stop();
    if (m_searchEdit) {
        m_searchEdit->blockSignals(true);
        m_searchEdit->clear();
        m_searchEdit->blockSignals(false);
    }

    showResultsGrid(
        books.isEmpty() ? QString("No books found in \"%1\"").arg(title)
                        : QString("%1 (%2 books)").arg(title).arg(books.size()),
        books);
}

void UserPanel::handleEditProfile(){
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
        req["userId"]      = m_userId;        // <-- ID, not username
        req["newUsername"] = newUsername;
        req["fullName"]    = newFullName;
        req["email"]       = newEmail;
        sendRequest(req);
    }
    if (dialog.wantsPasswordChange()) {
        QJsonObject req;
        req["action"]      = "user_change_password";
        req["userId"]      = m_userId;        // <-- ID here too
        req["oldPassword"] = dialog.oldPassword();
        req["newPassword"] = dialog.newPassword();
        sendRequest(req);
    }
}

void UserPanel::updateGenresLabel()
{
    if (!m_genresLabel) return;

    if (m_favoriteGenres.isEmpty()) {
        m_genresLabel->setText("Favorite genres: not set yet");
        return;
    }
    // Keep the sidebar tidy — show a few, then "+N more" if there are lots
    const int maxShown = 3;
    QStringList shown = m_favoriteGenres.mid(0, maxShown);
    QString text = "Favorite: " + shown.join(", ");
    if (m_favoriteGenres.size() > maxShown)
        text += QString(" +%1 more").arg(m_favoriteGenres.size() - maxShown);
    m_genresLabel->setText(text);
}

void UserPanel::saveFavoriteGenres()
{
    QJsonObject req;
    req["action"] = "user_set_favorite_genres";
    req["userId"] = m_userId;

    QJsonArray arr;
    for (const QString &genre : std::as_const(m_favoriteGenres))
        arr.append(genre);
    req["genres"] = arr;
    sendRequest(req);
}

void UserPanel::handleEditGenres()
{
    GenreSelectionDialog dialog(m_favoriteGenres, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    m_favoriteGenres = dialog.selectedGenres();
    updateGenresLabel();
    saveFavoriteGenres();
    rebuildHomeSections();
}

void UserPanel::openBookReader(int bookId)
{
    if (!m_ownedBookIds.contains(bookId)) {
        StyledMessageBox::error(this, "Not Purchased", "You need to purchase this book before you can read it.");
        return;
    }

    const QString remotePdfPath = m_ownedBookPdfPaths.value(bookId);
    if (remotePdfPath.isEmpty()) {
        StyledMessageBox::error(this, "Book Unavailable", "This book has no associated PDF file.");
        return;
    }

    QString title;
    for (const Book &b : std::as_const(m_storeBooks)) {
        if (b.id == bookId) { title = b.title; break; }
    }

    QFileInfo info(remotePdfPath);
    QDir().mkpath(QCoreApplication::applicationDirPath() + "/cache/books");
    const QString localPath = QCoreApplication::applicationDirPath() + "/cache/books/" + info.fileName();

    QString errorMsg;
    if (!downloadFileFromServer(remotePdfPath, localPath, errorMsg)) {
        StyledMessageBox::error(this, "Unable to Open Book",
                                errorMsg.isEmpty() ? "Failed to download this book's PDF file." : errorMsg);
        return;
    }

    m_pendingReaderBookId = bookId;
    m_pendingReaderLocalPath = localPath;
    m_pendingReaderTitle = title;

    QJsonObject req;
    req["action"] = "progress_fetch";
    req["userId"] = m_userId;
    req["bookId"] = bookId;
    sendRequest(req);
}

void UserPanel::launchPdfReader(int bookId, const QString &localPdfPath, const QString &title, int startPage)
{
    auto *reader = new PdfReaderDialog(localPdfPath, title, bookId, startPage, this);
    connect(reader, &PdfReaderDialog::readingProgressChanged, this, [this](int id, int lastPage, int pageCount) {
        m_readingProgressByBookId[id] = lastPage;
        m_totalPagesByBookId[id] = pageCount;

        QJsonObject req;
        req["action"] = "progress_update";
        req["userId"] = m_userId;
        req["bookId"] = id;
        req["lastPage"] = lastPage;
        sendRequest(req);

        rebuildContinueReadingItems();
    });
    reader->showFullScreen();
}

void UserPanel::rebuildContinueReadingItems()
{
    QVector<ContinueReadingItem> items;
    for (const Book &b : m_ownedBooksFull) {
        auto progIt = m_readingProgressByBookId.constFind(b.id);
        if (progIt == m_readingProgressByBookId.constEnd() || progIt.value() <= 0)
            continue;

        int totalPages = m_totalPagesByBookId.value(b.id, -1);
        if (totalPages < 0) {
            if (!m_pendingPageCountRequests.contains(b.id)) {
                m_pendingPageCountRequests.insert(b.id);
                QJsonObject req;
                req["action"] = "get_book_page_count";
                req["bookId"] = b.id;
                sendRequest(req);
            }
            continue;
        }

        int percent = totalPages > 0
                          ? qBound(0, ((progIt.value() + 1) * 100) / totalPages, 100)
                          : 0;
        if (totalPages > 0 && percent >= 100) continue;

        ContinueReadingItem item;
        item.book = b;
        item.lastPage = progIt.value();
        item.progressPercent = percent;
        items.push_back(item);
    }
    if (m_libraryPage) m_libraryPage->setContinueReading(items);
}

void UserPanel::updateStartupProgress(const QString &statusText, int loaded, int total)
{
    if (!m_startupLoaderActive || !m_startupLoader) return;

    if (!statusText.isEmpty() && m_loaderStatusLabel)
        m_loaderStatusLabel->setText(statusText);

    if (total > 0) {
        m_loaderProgressBar->setRange(0, total);
        m_loaderProgressBar->setValue(loaded);
    }

    if (total > 0 && loaded >= total) {
        m_startupLoaderActive = false;
        m_startupLoader->hide();
        m_startupLoader->deleteLater();
        m_startupLoader = nullptr;
    }
}

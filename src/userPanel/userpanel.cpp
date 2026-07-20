#include "userpanel.h"
#include "genreselectiondialog.h"
#include <QMessageBox>
#include <QPainter>
#include <QFrame>

static const char *kCardBg     = "#120E14";
static const char *kCardBorder = "#1F1724";
static const char *kAccent     = "#7C3E66";
static const char *kTextDim    = "#9A8FA0";

static QPixmap makeCoverPixmap(const Book &b, const QSize &size)
{
    QPixmap img(b.coverImagePath);
    if (!img.isNull())
        return img.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)
            .copy(0, 0, size.width(), size.height());

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

    connect(m_socket, &QTcpSocket::readyRead, this, &UserPanel::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &UserPanel::onSocketError);
    connect(m_socket, &QTcpSocket::connected, this, [this]() {
        requestAllBooks();
        m_cartPage->refreshCart();

        // Fetch User profile info & favorite genres
        QJsonObject reqUser;
        reqUser["action"] = "user_fetch";
        reqUser["username"] = m_username;
        sendRequest(reqUser);

        QJsonObject reqGenres;
        reqGenres["action"] = "user_get_favorite_genres";
        reqGenres["userId"] = m_userId;
        sendRequest(reqGenres);
    });

    m_socket->connectToHost("127.0.0.1", 1234);
    switchPage(0);
}

void UserPanel::sendRequest(const QJsonObject &requestObj)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, "Not connected", "Not connected to the server.");
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

    QLabel *avatarLabel = new QLabel("👤", avatarRow);
    avatarLabel->setStyleSheet("font-size: 40px; border: none; background: transparent;");

    QPushButton *editProfileBtn = new QPushButton("✏️", avatarRow);
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
    sidebarLayout->addSpacing(15);

    m_btnHome = new QPushButton("🏠 Home Discovery", sidebar);
    QString menuBtnStyle =
        "QPushButton { background-color: transparent; border: none; border-radius: 8px; padding: 10px; font-size: 13px; color: #9A8FA0; text-align: left; padding-left: 12px; }"
        "QPushButton:hover { background-color: #1F1724; color: #EAEAEA; }";
    m_btnHome->setStyleSheet(menuBtnStyle);
    m_btnHome->setCursor(Qt::PointingHandCursor);

    m_btnCart = new QPushButton("🛒 Shopping Cart", sidebar);
    m_btnCart->setStyleSheet(menuBtnStyle);
    m_btnCart->setCursor(Qt::PointingHandCursor);

    connect(m_btnHome, &QPushButton::clicked, this, [this]() { switchPage(0); });
    sidebarLayout->addWidget(m_btnHome);
    connect(m_btnCart, &QPushButton::clicked, this, [this]() { switchPage(1); });
    sidebarLayout->addWidget(m_btnCart);

    sidebarLayout->addStretch();

    m_btnLogout = new QPushButton("🚪 Logout", sidebar);
    m_btnLogout->setCursor(Qt::PointingHandCursor);
    m_btnLogout->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #7C3E66; border-radius: 8px; padding: 8px; font-weight: bold; color: #D9C2D1; text-align: left; padding-left: 12px; }"
        "QPushButton:hover { background-color: rgba(124, 62, 102, 60); color: white; border: 1px solid #B06B96; }"
        );
    connect(m_btnLogout, &QPushButton::clicked, this, &UserPanel::logoutRequested);
    sidebarLayout->addWidget(m_btnLogout);

    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->addWidget(createHomePage());    // index 0: home

    m_cartPage = new ShoppingCartPage(m_socket, m_userId, this);
    connect(m_cartPage, &ShoppingCartPage::cartUpdated, this, &UserPanel::updateHero);
    m_stackedWidget->addWidget(m_cartPage);           // index 1: cart

    m_detailsPage = new BookDetailsPage(this);
    m_stackedWidget->addWidget(m_detailsPage);        // index 2: details

    connect(m_detailsPage, &BookDetailsPage::backRequested, this, [this] { switchPage(0); });
    connect(m_detailsPage, &BookDetailsPage::addToCartRequested, this, &UserPanel::addToCart);
    connect(m_detailsPage, &BookDetailsPage::wishlistToggleRequested, this,
            [this](int id) { qDebug() << "wishlist toggle" << id; });
    connect(m_detailsPage, &BookDetailsPage::openBookRequested, this,
            [this](int id) { qDebug() << "open book" << id; });
    connect(m_detailsPage, &BookDetailsPage::reviewSubmitted, this,
            [this](int id, int rating, const QString &text) {
                qDebug() << "review" << id << rating << text;
            });
    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(m_stackedWidget);
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

    if (index == 1) {
        m_cartPage->refreshCart();
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
    m_searchEdit->setPlaceholderText("🔍  Search by title, author, or publisher...");
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
    m_searchResultsLabel = new QLabel(m_searchResultsPanel);
    m_searchResultsLabel->setStyleSheet("color:#FFFFFF;font-size:16px;font-weight:bold;border:none;background:transparent;");
    srLayout->addWidget(m_searchResultsLabel);
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
            "QPushButton{background-color:#1F1724;border:none;border-radius:20px;"
            "color:#EAEAEA;font-size:16px;font-weight:bold;}"
            "QPushButton:hover{background-color:#7C3E66;}");
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
        "QPushButton{background:transparent;border:1px solid #3A3244;border-radius:8px;"
        "padding:10px 20px;color:#EAEAEA;font-size:13px;}"
        "QPushButton:hover{border-color:#7C3E66;background-color:#1A141F;}");
    m_heroCartBtn = new QPushButton("Add to Cart", hero);
    m_heroCartBtn->setStyleSheet(QString(
                               "QPushButton{background-color:%1;border:none;border-radius:8px;"
                               "padding:10px 20px;color:white;font-size:13px;font-weight:bold;}"
                               "QPushButton:hover{background-color:#B06B96;}").arg(kAccent));
    viewBtn->setCursor(Qt::PointingHandCursor);
    m_heroCartBtn->setCursor(Qt::PointingHandCursor);
    heroBtns->addWidget(viewBtn); heroBtns->addWidget(m_heroCartBtn); heroBtns->addStretch();

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
    layout->addWidget(makeHorizontalScrollRow("Recommended for You", m_rowRecommended));
    layout->addWidget(makeHorizontalScrollRow("New Releases", m_rowNewReleases));
    layout->addWidget(makeHorizontalScrollRow("Bestsellers", m_rowBestsellers));
    layout->addWidget(makeHorizontalScrollRow("Free Books", m_rowFree));

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

    auto *cover = new QLabel(card);
    cover->setFixedSize(112, 150);
    cover->setPixmap(makeCoverPixmap(b, QSize(112, 150)));
    cover->setScaledContents(true);

    auto *title = new QLabel(b.title, card);
    title->setWordWrap(true);

    auto *author = new QLabel(b.author, card);

    v->addWidget(cover);
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
    fillBookRow(m_rowRecommended, recommended);

    QVector<Book> newest = m_storeBooks;
    std::sort(newest.begin(), newest.end(),
              [](const Book &a, const Book &b) { return a.id > b.id; });
    fillBookRow(m_rowNewReleases, newest);

    QVector<Book> best = m_storeBooks;
    std::sort(best.begin(), best.end(),
              [](const Book &a, const Book &b) { return a.totalSales > b.totalSales; });
    fillBookRow(m_rowBestsellers, best);

    QVector<Book> free;
    for (const Book &b : std::as_const(m_storeBooks))
        if (b.price <= 0.0) free.push_back(b);
    qDebug() << "Free books found:" << free.size() << "| Total store books:" << m_storeBooks.size();
    fillBookRow(m_rowFree, free);
    qDebug() << "Books received:" << m_storeBooks.size();

    for (const Book &b : m_storeBooks)
    {
        qDebug() << b.id << b.title;
    }

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
        QString type = responseObj["type"].toString();

        if (responseObj.contains("action")) {
            m_cartPage->handleServerResponse(responseObj);
        }

        if (responseObj.contains("books") && responseObj["status"].toString() == "success") {
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
                m_storeBooks.push_back(b);
            }
            rebuildHomeSections();
        }
        else if (type == "favorite_genres" && responseObj["success"].toBool()) {
            m_favoriteGenres.clear();
            for (const QJsonValue &v : responseObj["genres"].toArray()) {
                m_favoriteGenres << v.toString();
            }

            if (m_favoriteGenres.isEmpty()) {
                GenreSelectionDialog dialog(this);
                if (dialog.exec() == QDialog::Accepted) {
                    m_favoriteGenres = dialog.selectedGenres();

                    QJsonObject req;
                    req["action"] = "user_set_favorite_genres";
                    req["userId"] = m_userId;

                    QJsonArray arr;
                    for (const QString &genre : m_favoriteGenres) {
                        arr.append(genre);
                    }
                    req["genres"] = arr;
                    sendRequest(req);
                }
            }

            rebuildHomeSections();
        }
        else if (type == "favorite_genres_saved" && responseObj["success"].toBool()) {
            rebuildHomeSections();
        }
        else if (type == "user_info" && responseObj["status"].toString() == "success") {
            m_fullName = responseObj["fullName"].toString();
            m_email = responseObj["email"].toString();
            m_nameLabel->setText(m_fullName.isEmpty() ? m_username : m_fullName);
        }
    }
}

void UserPanel::onSocketError()
{
    qWarning() << "UserPanel socket error: " << m_socket->errorString();
}

void UserPanel::handleEditProfile()
{
    qDebug() << "Open profile update UI workspace window dialog context.";
}

QWidget *UserPanel::makeHorizontalScrollRow(const QString &title, QHBoxLayout *&rowLayoutOut)
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
    connect(seeAllBtn, &QPushButton::clicked, this, [this, title]() {
        qDebug() << "See all:" << title;
        // TODO: open full grid page for this category
    });

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(seeAllBtn);
    vLayout->addLayout(headerLayout);

    // Horizontal scroll area
    auto *scrollArea = new QScrollArea(container);
    scrollArea->setWidgetResizable(true);  // critical for horizontal
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setFixedHeight(260);  // card height + padding
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
    rowLayoutOut->setContentsMargins(0, 0, 0, 0);
    rowLayoutOut->setSpacing(16);
    scrollContent->setMinimumHeight(250);

    rowLayoutOut->setContentsMargins(10, 10, 10, 10);
    rowLayoutOut->setSpacing(16);
    rowLayoutOut->setAlignment(Qt::AlignLeft);

    scrollArea->setWidget(scrollContent);
    vLayout->addWidget(scrollArea);

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

    if (m_cartPage && m_cartPage->containsBook(b.id)) {
        m_heroCartBtn->setText("✓ In Cart");
        m_heroCartBtn->setStyleSheet(
            "QPushButton{background-color:#2A4D3B;border:none;border-radius:8px;"
            "padding:10px 20px;color:white;font-size:13px;font-weight:bold;}");
    } else {
        m_heroCartBtn->setText("Add to Cart");
        m_heroCartBtn->setStyleSheet(QString(
                                         "QPushButton{background-color:%1;border:none;border-radius:8px;"
                                         "padding:10px 20px;color:white;font-size:13px;font-weight:bold;}"
                                         "QPushButton:hover{background-color:#B06B96;}").arg(kAccent));
    }
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

    // empty query → back to normal home page
    if (q.isEmpty()) {
        m_searchResultsPanel->hide();
        m_homeSections->show();
        return;
    }

    // match title OR author OR publisher, case-insensitive
    QVector<Book> results;
    for (const Book &b : std::as_const(m_storeBooks)) {
        if (b.title.contains(q, Qt::CaseInsensitive)
            || b.author.contains(q, Qt::CaseInsensitive)
            || b.publisherName.contains(q, Qt::CaseInsensitive)) {
            results.push_back(b);
        }
    }

    // clear old grid
    while (QLayoutItem *it = m_searchResultsGrid->takeAt(0)) {
        if (it->widget()) it->widget()->deleteLater();
        delete it;
    }

    m_searchResultsLabel->setText(
        results.isEmpty()
            ? QString("No results for \"%1\"").arg(q)
            : QString("Results for \"%1\"  (%2 found)").arg(q).arg(results.size()));

    const int columns = 5;
    int row = 0, col = 0;
    for (const Book &b : std::as_const(results)) {
        m_searchResultsGrid->addWidget(makeBookCard(b), row, col);
        if (++col >= columns) { col = 0; ++row; }
    }

    m_homeSections->hide();
    m_searchResultsPanel->show();
}


void UserPanel::addToCart(int bookId)       {
    QJsonObject req;
    req["action"] = "add_to_cart";
    req["userId"] = m_userId;
    req["bookId"] = bookId;
    req["quantity"] = 1;

    sendRequest(req);

    QMessageBox::information(this, "Added to Cart", "Book successfully added to your cart!");
}

void UserPanel::openBookDetails(int bookId)
{
    for (const Book &b : std::as_const(m_storeBooks)) {
        if (b.id == bookId) {
            m_detailsPage->setBook(b);
            switchPage(2);   // check this is 2, not 1
            return;
        }
    }
}

void UserPanel::openGenre(const QString &g) { qDebug() << "genre" << g; }
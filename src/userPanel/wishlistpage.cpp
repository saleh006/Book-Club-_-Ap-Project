#include "wishlistpage.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QScrollArea>
#include <QScroller>
#include <QLineEdit>
#include <QComboBox>
#include <QResizeEvent>
#include <QMessageBox>
#include <QJsonDocument>
#include <QTcpSocket>
#include <QSet>

WishlistItemWidget::WishlistItemWidget(const WishlistDisplayItem &item, bool listMode, QWidget *parent)
    : QFrame(parent), m_item(item), m_listMode(listMode)
{
    setObjectName("wishlistItemCard");
    setCursor(Qt::PointingHandCursor);
    setStyleSheet(
                "QFrame#wishlistItemCard { background-color: #17121b; border: 1px solid #241d2b; border-radius: 12px; }"
                "QFrame#wishlistItemCard:hover { border-color: #e0559e; background-color: #1c151f; }"
                "QLabel { background: transparent; border: none; }");

    if (m_listMode)
        buildListUi();
    else
        buildGridUi();
}

void WishlistItemWidget::mousePressEvent(QMouseEvent *event)
{
    QFrame::mousePressEvent(event);
    emit detailsRequested(m_item.bookId);
}

QLabel *WishlistItemWidget::makeCoverLabel(QWidget *parent, const QSize &size)
{
    auto *cover = new QLabel(parent);
    cover->setFixedSize(size);
    cover->setAlignment(Qt::AlignCenter);
    cover->setWordWrap(true);
    cover->setStyleSheet(
        "background-color: #2e2735; border-radius: 8px; color: #9d94a6; font-size: 11px; padding: 4px;");
    if (!m_item.cover.isNull())
        cover->setPixmap(m_item.cover.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)
                             .copy(0, 0, size.width(), size.height()));
    else
        cover->setText(m_item.title);
    return cover;
}

QPushButton *WishlistItemWidget::makeHeartButton(QWidget *parent)
{
    auto *heart = new QPushButton(QStringLiteral("♥"), parent);
    heart->setCursor(Qt::PointingHandCursor);
    heart->setFixedSize(28, 28);
    heart->setToolTip(tr("Remove from Wishlist"));
    heart->setStyleSheet(
                    "QPushButton { color: #e0559e; background-color: rgba(16, 13, 19, 170); border: none;"
                    " border-radius: 14px; font-size: 15px; }"
                    "QPushButton:hover { background-color: rgba(224, 85, 158, 60); }");
    connect(heart, &QPushButton::clicked, this, [this] { emit removeRequested(m_item.bookId); });
    return heart;
}

QPushButton *WishlistItemWidget::makeCartButton()
{
    auto *cartBtn = new QPushButton(QStringLiteral("🛒"), this);
    cartBtn->setCursor(Qt::PointingHandCursor);
    cartBtn->setToolTip(tr("Add to Cart"));
    cartBtn->setFixedHeight(34);
    cartBtn->setStyleSheet(
                        "QPushButton { color: #ffffff; background-color: #7C3E66; border: none;"
                        " border-radius: 8px; font-size: 14px; }"
                        "QPushButton:hover { background-color: #B06B96; }");
    connect(cartBtn, &QPushButton::clicked, this, [this] { emit addToCartRequested(m_item.bookId); });
    return cartBtn;
}

QPushButton *WishlistItemWidget::makeRemoveButton()
{
    auto *removeBtn = new QPushButton(QStringLiteral("🗑"), this);
    removeBtn->setCursor(Qt::PointingHandCursor);
    removeBtn->setToolTip(tr("Remove from Wishlist"));
    removeBtn->setFixedHeight(34);
    removeBtn->setStyleSheet(
        "QPushButton { color: #c9c2d1; background-color: #1c151f; border: 1px solid #2f2636;"
        " border-radius: 8px; font-size: 13px; }"
        "QPushButton:hover { border-color: rgba(228, 96, 96, 0.55); color: #e46060;"
        " background-color: rgba(228, 96, 96, 0.10); }");
    connect(removeBtn, &QPushButton::clicked, this, [this] { emit removeRequested(m_item.bookId); });
    return removeBtn;
}

void WishlistItemWidget::buildGridUi()
{
    setFixedWidth(220);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(15, 15, 15, 15);
    rootLayout->setSpacing(6);

    auto *coverContainer = new QWidget(this);
    coverContainer->setFixedSize(188, 244);
    coverContainer->setStyleSheet("background: transparent; border: none;");

    auto *cover = makeCoverLabel(coverContainer, QSize(188, 244));
    cover->setGeometry(0, 0, 188, 244);

    if (m_item.isNew) {
        auto *newBadge = new QLabel(tr("New"), coverContainer);
        newBadge->setStyleSheet(
            "background-color: rgba(16, 13, 19, 200); color: #f4f1f6; border: none;"
            " border-radius: 9px; font-size: 11px; font-weight: 700; padding: 2px 10px;");
        newBadge->adjustSize();
        newBadge->move(8, 8);
        newBadge->raise();
    }

    auto *heart = makeHeartButton(coverContainer);
    heart->move(coverContainer->width() - heart->width() - 6, 6);
    heart->raise();

    rootLayout->addWidget(coverContainer, 0, Qt::AlignHCenter);
    rootLayout->addSpacing(4);

    auto *titleLabel = new QLabel(m_item.title, this);
    titleLabel->setWordWrap(true);
    titleLabel->setAlignment(Qt::AlignHCenter);
    titleLabel->setStyleSheet("color: #f4f1f6; font-size: 15px; font-weight: 700;");
    rootLayout->addWidget(titleLabel);

    auto *authorLabel = new QLabel(m_item.author, this);
    authorLabel->setAlignment(Qt::AlignHCenter);
    authorLabel->setStyleSheet("color: #9A8FA0; font-size: 13px;");
    rootLayout->addWidget(authorLabel);

    auto *formatLabel = new QLabel(QStringLiteral("📖  %1").arg(m_item.format), this);
    formatLabel->setAlignment(Qt::AlignHCenter);
    formatLabel->setStyleSheet("color: #9A8FA0; font-size: 12px;");
    rootLayout->addWidget(formatLabel);

    auto *priceLabel = new QLabel(m_item.price <= 0.0 ? tr("Free") : QStringLiteral("$%1").arg(m_item.price, 0, 'f', 2), this);
    priceLabel->setAlignment(Qt::AlignHCenter);
    priceLabel->setStyleSheet("color: #e0559e; font-size: 16px; font-weight: 700;");
    rootLayout->addWidget(priceLabel);

    rootLayout->addStretch();

    auto *actionRow = new QHBoxLayout;
    actionRow->setContentsMargins(0, 4, 0, 0);
    actionRow->setSpacing(10);
    actionRow->addWidget(makeCartButton(), 1);
    auto *removeBtn = makeRemoveButton();
    removeBtn->setFixedWidth(64);
    actionRow->addWidget(removeBtn);
    rootLayout->addLayout(actionRow);
}

void WishlistItemWidget::buildListUi()
{
    setFixedHeight(128);

    auto *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(18);

    auto *coverContainer = new QWidget(this);
    coverContainer->setFixedSize(72, 96);
    coverContainer->setStyleSheet("background: transparent; border: none;");
    auto *cover = makeCoverLabel(coverContainer, QSize(72, 96));
    cover->setGeometry(0, 0, 72, 96);
    rootLayout->addWidget(coverContainer,0,Qt::AlignVCenter);

    auto *infoLayout = new QVBoxLayout;
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(4);
    infoLayout->addStretch();

    auto *titleLabel = new QLabel(m_item.title, this);
    titleLabel->setStyleSheet("color: #f4f1f6; font-size: 16px; font-weight: 700;");
    titleLabel->setWordWrap(true);
    infoLayout->addWidget(titleLabel);

    auto *authorLabel = new QLabel(m_item.author, this);
    authorLabel->setStyleSheet("color: #9A8FA0; font-size: 13px;");
    infoLayout->addWidget(authorLabel);

    auto *formatLabel = new QLabel(QStringLiteral("📖  %1").arg(m_item.format), this);
    formatLabel->setStyleSheet("color: #9A8FA0; font-size: 12px;");
    infoLayout->addWidget(formatLabel);
    infoLayout->addStretch();
    rootLayout->addLayout(infoLayout, 1);

    auto *rightLayout = new QVBoxLayout;
    rightLayout->setSpacing(10);
    rightLayout->addStretch();

    auto *priceLabel = new QLabel(m_item.price <= 0.0 ? tr("Free") : QStringLiteral("$%1").arg(m_item.price, 0, 'f', 2), this);
    priceLabel->setAlignment(Qt::AlignRight);
    priceLabel->setStyleSheet("color: #e0559e; font-size: 16px; font-weight: 700;");
    rightLayout->addWidget(priceLabel);

    auto *actionRow = new QHBoxLayout;
    actionRow->setSpacing(10);
    auto *cartBtn = makeCartButton();
    cartBtn->setFixedWidth(64);
    auto *removeBtn = makeRemoveButton();
    removeBtn->setFixedWidth(48);
    actionRow->addStretch();
    actionRow->addWidget(cartBtn);
    actionRow->addWidget(removeBtn);
    rightLayout->addLayout(actionRow);
    rightLayout->addStretch();

    rootLayout->addLayout(rightLayout);
}

WishlistPage::WishlistPage(QTcpSocket *socket, int userId, QWidget *parent)
    : QWidget(parent), m_socket(socket), m_userId(userId)
{
    buildUi();
}

void WishlistPage::buildUi()
{
    setObjectName("wishlistPage");
    setStyleSheet("QWidget#wishlistPage { background-color: #100d13; }");

    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(28, 24, 28, 14);
    pageLayout->setSpacing(0);

    auto *headerLayout = new QHBoxLayout;
    headerLayout->setSpacing(12);

    auto *titleColumn = new QVBoxLayout;
    titleColumn->setSpacing(6);
    auto *titleRow = new QHBoxLayout;
    titleRow->setSpacing(10);
    auto *titleLabel = new QLabel(tr("My Wishlist"), this);
    titleLabel->setStyleSheet("color: #f4f1f6; font-size: 28px; font-weight: 800; background: transparent;");
    auto *titleHeart = new QLabel(QStringLiteral("♡"), this);
    titleHeart->setStyleSheet("color: #e0559e; font-size: 26px; font-weight: 700; background: transparent;");
    titleRow->addWidget(titleLabel);
    titleRow->addWidget(titleHeart);
    titleRow->addStretch();
    titleColumn->addLayout(titleRow);

    auto *subtitleLabel = new QLabel(tr("Books you love and want to read later."), this);
    subtitleLabel->setStyleSheet("color: #9A8FA0; font-size: 14px; background: transparent;");
    titleColumn->addWidget(subtitleLabel);
    headerLayout->addLayout(titleColumn, 1);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search in wishlist...  🔍"));
    m_searchEdit->setFixedSize(280, 44);
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(
                        "QLineEdit { background-color: #17121b; border: 1px solid #241d2b; border-radius: 10px;"
                        " padding-left: 14px; padding-right: 14px; color: #f4f1f6; font-size: 13px; }"
                        "QLineEdit:focus { border-color: #e0559e; background-color: #1c151f; }");
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this] { rebuildGrid(); });
    headerLayout->addWidget(m_searchEdit, 0, Qt::AlignVCenter);

    m_filterButton = new QPushButton(QStringLiteral("▼"), this);
    m_filterButton->setCursor(Qt::PointingHandCursor);
    m_filterButton->setFixedSize(44, 44);
    m_filterButton->setToolTip(tr("Filter: All"));
    m_filterButton->setStyleSheet(
                            "QPushButton { color: #e0559e; background-color: #17121b; border: 1px solid #241d2b;"
                            " border-radius: 10px; font-size: 14px; }"
                            "QPushButton:hover { border-color: #e0559e; }");
    connect(m_filterButton, &QPushButton::clicked, this, [this] {
        m_filterMode = (m_filterMode + 1) % 3;
        static const QString names[] = { tr("All"), tr("Free only"), tr("Paid only") };
        m_filterButton->setToolTip(tr("Filter: %1").arg(names[m_filterMode]));
        rebuildGrid();
    });
    headerLayout->addWidget(m_filterButton, 0, Qt::AlignVCenter);
    pageLayout->addLayout(headerLayout);
    pageLayout->addSpacing(18);

    auto *divider = new QFrame(this);
    divider->setFrameShape(QFrame::HLine);
    divider->setFixedHeight(1);
    divider->setStyleSheet("background-color: #241d2b; border: none;");
    pageLayout->addWidget(divider);
    pageLayout->addSpacing(16);

    auto *toolbarLayout = new QHBoxLayout;
    toolbarLayout->setSpacing(12);
    m_countLabel = new QLabel(tr("0 books"), this);
    m_countLabel->setStyleSheet("color: #f4f1f6; font-size: 14px; background: transparent;");
    toolbarLayout->addWidget(m_countLabel);
    toolbarLayout->addStretch();

    m_sortCombo = new QComboBox(this);
    m_sortCombo->setCursor(Qt::PointingHandCursor);
    m_sortCombo->setFixedHeight(38);
    m_sortCombo->addItem(tr("Sort by: Recently Added"));
    m_sortCombo->addItem(tr("Sort by: Title A–Z"));
    m_sortCombo->addItem(tr("Sort by: Price ↑"));
    m_sortCombo->addItem(tr("Sort by: Price ↓"));
    m_sortCombo->setStyleSheet(
                        "QComboBox { background-color: #17121b; border: 1px solid #241d2b; border-radius: 8px;"
                        " padding: 6px 14px; color: #c9c2d1; font-size: 13px; }"
                        "QComboBox:hover { border-color: #3a3341; }"
                        "QComboBox::drop-down { border: none; width: 24px; }"
                        "QComboBox QAbstractItemView { background-color: #17121b; border: 1px solid #241d2b;"
                        " color: #c9c2d1; selection-background-color: #7C3E66; }");
    connect(m_sortCombo, &QComboBox::currentIndexChanged, this, [this] { rebuildGrid(); });
    toolbarLayout->addWidget(m_sortCombo);

    auto makeViewButton = [this](const QString &text) {
        auto *btn = new QPushButton(text, this);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedSize(38, 38);
        return btn;
    };
    m_gridViewButton = makeViewButton(QStringLiteral("▦"));
    m_listViewButton = makeViewButton(QStringLiteral("☰"));
    connect(m_gridViewButton, &QPushButton::clicked, this, [this] { setListMode(false); });
    connect(m_listViewButton, &QPushButton::clicked, this, [this] { setListMode(true); });
    toolbarLayout->addWidget(m_gridViewButton);
    toolbarLayout->addWidget(m_listViewButton);
    pageLayout->addLayout(toolbarLayout);
    pageLayout->addSpacing(16);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { background: transparent; width: 8px; margin: 0; }"
        "QScrollBar::handle:vertical { background-color: #3a3341; border-radius: 4px; min-height: 32px; }"
        "QScrollBar::handle:vertical:hover { background-color: #4a4253; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }");
    QScroller::grabGesture(m_scrollArea->viewport(), QScroller::TouchGesture);

    auto *gridContainer = new QWidget(m_scrollArea);
    gridContainer->setStyleSheet("background: transparent;");
    auto *containerLayout = new QVBoxLayout(gridContainer);
    containerLayout->setContentsMargins(0, 0, 8, 0);
    containerLayout->setSpacing(0);

    m_gridLayout = new QGridLayout;
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setHorizontalSpacing(18);
    m_gridLayout->setVerticalSpacing(18);
    m_gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    containerLayout->addLayout(m_gridLayout);

    m_emptyLabel = new QLabel(gridContainer);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setWordWrap(true);
    m_emptyLabel->setStyleSheet("color: #9A8FA0; font-size: 15px; padding: 40px; background: transparent;");
    containerLayout->addWidget(m_emptyLabel);
    containerLayout->addStretch();

    m_discoverCard = makeDiscoverCard();
    m_discoverCard->setParent(gridContainer);
    m_discoverCard->hide();

    m_scrollArea->setWidget(gridContainer);
    pageLayout->addWidget(m_scrollArea, 1);

    auto *tipLabel = new QLabel(tr("💡 Tip: Add books to your wishlist to keep track of what you want to read."), this);
    tipLabel->setAlignment(Qt::AlignCenter);
    tipLabel->setStyleSheet("color: #9A8FA0; font-size: 12px; padding-top: 10px; background: transparent;");
    pageLayout->addWidget(tipLabel);

    setListMode(false);
    updateEmptyLabel(0);
}

QWidget *WishlistPage::makeDiscoverCard()
{
    auto *card = new QFrame;
    card->setObjectName("discoverCard");
    card->setFixedWidth(220);
    card->setStyleSheet(
                    "QFrame#discoverCard { background: transparent; border: 2px dashed rgba(224, 85, 158, 0.45);"
                    " border-radius: 12px; }"
                    "QFrame#discoverCard:hover { border-color: #e0559e; }"
                    "QLabel { background: transparent; border: none; }");

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 24, 18, 24);
    layout->setSpacing(8);
    layout->addStretch();

    auto *plusLabel = new QLabel(QStringLiteral("＋"), card);
    plusLabel->setAlignment(Qt::AlignCenter);
    plusLabel->setFixedSize(56, 56);
    plusLabel->setStyleSheet(
                    "background-color: rgba(124, 62, 102, 120); color: #e0559e; border: none;"
                    " border-radius: 28px; font-size: 26px; font-weight: 700;");
    layout->addWidget(plusLabel, 0, Qt::AlignHCenter);
    layout->addSpacing(6);

    auto *titleLabel = new QLabel(tr("Discover more books"), card);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #f4f1f6; font-size: 15px; font-weight: 700;");
    layout->addWidget(titleLabel);

    auto *subLabel = new QLabel(tr("Explore new arrivals and add them to your wishlist."), card);
    subLabel->setAlignment(Qt::AlignCenter);
    subLabel->setWordWrap(true);
    subLabel->setStyleSheet("color: #9A8FA0; font-size: 12px;");
    layout->addWidget(subLabel);
    layout->addSpacing(10);

    auto *exploreBtn = new QPushButton(tr("Explore Books"), card);
    exploreBtn->setCursor(Qt::PointingHandCursor);
    exploreBtn->setFixedHeight(34);
    exploreBtn->setStyleSheet(
                        "QPushButton { color: #f4f1f6; background: transparent; border: 1px solid #e0559e;"
                        " border-radius: 8px; padding: 6px 16px; font-size: 12px; }"
                        "QPushButton:hover { background-color: rgba(224, 85, 158, 0.12); }");
    connect(exploreBtn, &QPushButton::clicked, this, &WishlistPage::exploreBooksRequested);
    layout->addWidget(exploreBtn, 0, Qt::AlignHCenter);
    layout->addStretch();

    return card;
}

void WishlistPage::setCatalog(const QVector<Book> &books) {
    m_catalog = books;
    for (WishlistDisplayItem &item : m_items)
        enrichFromCatalog(item);
    rebuildGrid();
}

void WishlistPage::enrichFromCatalog(WishlistDisplayItem &item) const
{
    for (const Book &b : m_catalog) {
        if (b.id != item.bookId)
            continue;
        item.price = b.price;
        item.genre = b.genre;
        if (item.cover.isNull() && !b.coverImagePath.isEmpty()) {
            QPixmap pm(b.coverImagePath);
            if (!pm.isNull())
                item.cover = pm;
        }
        break;
    }
}

void WishlistPage::setItems(const QList<WishlistDisplayItem> &items)
{
    m_items = items;
    rebuildGrid();
    emit wishlistUpdated();
}

void WishlistPage::removeItemLocally(int bookId)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].bookId == bookId) {
            m_items.removeAt(i);
            break;
        }
    }
    rebuildGrid();
    emit wishlistUpdated();
}

void WishlistPage::setListMode(bool listMode)
{
    m_listMode = listMode;
    const QString activeStyle = QString(
                                    "QPushButton { color: #ffffff; background-color: #7C3E66; border: none;"
                                    " border-radius: 8px; font-size: 15px; }");
    const QString normalStyle = QString(
                                    "QPushButton { color: #9A8FA0; background-color: #17121b; border: 1px solid #241d2b;"
                                    " border-radius: 8px; font-size: 15px; }"
                                    "QPushButton:hover { color: #f4f1f6; border-color: #3a3341; }");

    m_gridViewButton->setStyleSheet(listMode ? normalStyle : activeStyle);
    m_listViewButton->setStyleSheet(listMode ? activeStyle : normalStyle);
    rebuildGrid();
}

QList<WishlistDisplayItem> WishlistPage::filteredSortedItems() const
{
    const QString query = m_searchEdit->text().trimmed();

    QList<WishlistDisplayItem> result;
    for (const WishlistDisplayItem &item : m_items) {
        if (m_filterMode == 1 && item.price > 0.0) continue;
        if (m_filterMode == 2 && item.price <= 0.0) continue;
        if (query.isEmpty()
            || item.title.contains(query, Qt::CaseInsensitive)
            || item.author.contains(query, Qt::CaseInsensitive)) {
            result.append(item);
        }
    }

    switch (m_sortCombo->currentIndex()) {
    case 1:
        std::sort(result.begin(), result.end(), [](const WishlistDisplayItem &a, const WishlistDisplayItem &b) {
            return QString::compare(a.title, b.title, Qt::CaseInsensitive) < 0;
        });
        break;
    case 2:
        std::sort(result.begin(), result.end(), [](const WishlistDisplayItem &a, const WishlistDisplayItem &b) { return a.price < b.price; });
        break;
    case 3:
        std::sort(result.begin(), result.end(), [](const WishlistDisplayItem &a, const WishlistDisplayItem &b) { return a.price > b.price; });
        break;
    default:
        std::reverse(result.begin(), result.end());
        break;
    }
    return result;
}

int WishlistPage::gridColumnCount() const
{
    if (m_listMode)
        return 1;
    const int cardWidth = 220 + m_gridLayout->horizontalSpacing();
    const int available = qMax(1, m_scrollArea->viewport()->width() - 8);
    return qBound(1, available / cardWidth, 6);
}

void WishlistPage::rebuildGrid()
{
    const auto oldCards = m_scrollArea->widget()->findChildren<WishlistItemWidget *>();
    for (WishlistItemWidget *card : oldCards)
        card->deleteLater();

    QWidget *gridContainer = m_scrollArea->widget();
    auto *containerLayout = qobject_cast<QVBoxLayout *>(gridContainer->layout());

    while (QLayoutItem *item = m_gridLayout->takeAt(0))
        delete item;

    containerLayout->removeItem(m_gridLayout);
    delete m_gridLayout;

    m_gridLayout = new QGridLayout;
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setHorizontalSpacing(18);
    m_gridLayout->setVerticalSpacing(18);
    m_gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    containerLayout->insertLayout(0, m_gridLayout);

    const QList<WishlistDisplayItem> visible = filteredSortedItems();
    m_columns = gridColumnCount();

    int row = 0, col = 0;
    for (const WishlistDisplayItem &item : visible) {
        auto *card = new WishlistItemWidget(item, m_listMode, m_scrollArea->widget());
        connect(card, &WishlistItemWidget::removeRequested, this, [this](int bookId) {
            removeItemLocally(bookId);
            sendRequest("wishlist_remove", {{"bookId", bookId}});
        });
        connect(card, &WishlistItemWidget::addToCartRequested, this, [this](int bookId) {
            removeItemLocally(bookId);
            sendRequest("wishlist_remove", {{"bookId", bookId}});
            emit addToCartRequested(bookId);
        });
        connect(card, &WishlistItemWidget::detailsRequested, this, &WishlistPage::bookDetailsRequested);

        if (m_listMode)
            m_gridLayout->addWidget(card, row++, 0);
        else {
            m_gridLayout->addWidget(card, row, col);
            if (++col >= m_columns) { col = 0; ++row; }
        }
    }

    const bool showDiscover = !m_listMode && !m_items.isEmpty();
    m_discoverCard->setVisible(showDiscover);
    if (showDiscover)
        m_gridLayout->addWidget(m_discoverCard, row, col);

    refreshCountLabel();
    updateEmptyLabel(visible.size());
}

void WishlistPage::refreshCountLabel()
{
    m_countLabel->setText(tr("%1 %2").arg(m_items.size()).arg(m_items.size() == 1 ? tr("book") : tr("books")));
}

void WishlistPage::updateEmptyLabel(int visibleCount)
{
    if (m_items.isEmpty())
        m_emptyLabel->setText(tr("💜\n\nYour wishlist is empty.\nBrowse the store and tap the heart on any book to save it here."));
    else
        m_emptyLabel->setText(tr("No wishlist items match your search."));
    m_emptyLabel->setVisible(visibleCount == 0);
}

void WishlistPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (gridColumnCount() != m_columns)
        rebuildGrid();
}

void WishlistPage::sendRequest(const QString &action, const QJsonObject &extra)
{
    if (!m_socket)
        return;

    QJsonObject packet = extra;
    packet["action"] = action;
    packet["userId"] = m_userId;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
    m_socket->flush();
}

void WishlistPage::refreshWishlist() {
    sendRequest("wishlist_fetch");
}

void WishlistPage::handleServerResponse(const QJsonObject &response)
{
    const QString action = response["action"].toString();

    if (action == "wishlist_fetch_response") {
        handleWishlistFetchResponse(response);
    } else if (action == "wishlist_add_response"
               || action == "wishlist_remove_response") {
        handleMutationResponse(response);
    }
}

void WishlistPage::handleWishlistFetchResponse(const QJsonObject &response)
{
    if (response["status"].toString() != "success") {
        QMessageBox::warning(this, tr("Wishlist"), response["message"].toString());
        return;
    }

    QSet<int> previousIds;
    for (const WishlistDisplayItem &existing : std::as_const(m_items))
        previousIds.insert(existing.bookId);

    QList<WishlistDisplayItem> items;
    const QJsonArray rawBooks = response["books"].toArray();
    for (const QJsonValue &value : rawBooks) {
        const QJsonObject o = value.toObject();
        WishlistDisplayItem item;
        item.bookId = o["id"].toInt();
        item.title = o["title"].toString();
        item.author = o["author"].toString();
        enrichFromCatalog(item);
        item.isNew = !previousIds.contains(item.bookId);
        items.append(item);
    }
    setItems(items);
}

void WishlistPage::handleMutationResponse(const QJsonObject &response)
{
    if (response["status"].toString() != "success") {
        QMessageBox::warning(this, tr("Wishlist"), response["message"].toString());
        refreshWishlist();
        return;
    }

    refreshWishlist();
}
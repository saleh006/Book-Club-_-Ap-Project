#include "mylibrarypage.h"
#include "statisticscard.h"
#include "shelfdetailpage.h"
#include "librarydialogs.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QButtonGroup>
#include <QScrollArea>
#include <QStackedWidget>
#include <QResizeEvent>
#include <QFrame>
#include <algorithm>

namespace {
const char *kTabButtonStyle =
    "QPushButton{background:transparent;border:none;border-radius:8px;"
    "color: rgba(255,255,255,150);font-size:13px;font-weight:600;padding:0 18px;}"
    "QPushButton:checked{background-color:#A855F7;color:white;}"
    "QPushButton:hover:!checked{color:#FFFFFF;background-color:rgba(255,255,255,15);}";

const char *kViewToggleStyle =
    "QPushButton{background:transparent;border:none;border-radius:7px;"
    "color: rgba(255,255,255,150);font-size:13px;}"
    "QPushButton:checked{background-color:#A855F7;color:white;}"
    "QPushButton:hover:!checked{color:#FFFFFF;background-color:rgba(255,255,255,15);}";
}

MyLibraryPage::MyLibraryPage(QWidget *parent) : QWidget(parent)
{
    setStyleSheet("background-color:#09070D; font-family:'Segoe UI';");

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_stack = new QStackedWidget(this);

    // ---- page 0: the library itself, scrollable ----
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(
        "QScrollArea{border:none;background:#09070D;}"
        "QScrollBar:vertical{width:8px;background:#120E14;border-radius:4px;}"
        "QScrollBar::handle:vertical{background:#A855F7;border-radius:4px;min-height:30px;}"
        "QScrollBar::handle:vertical:hover{background:#C084FC;}");

    auto *page = new QWidget;
    page->setStyleSheet("background:#09070D;");
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 20, 30, 30);
    layout->setSpacing(24);

    layout->addWidget(buildHeader());
    layout->addWidget(buildTabsAndToolbar());

    m_continueSection = buildContinueReadingSection();
    layout->addWidget(m_continueSection);

    m_booksSection = buildMyBooksSection();
    layout->addWidget(m_booksSection);

    m_shelvesSection = buildShelvesSection();
    layout->addWidget(m_shelvesSection);

    layout->addStretch();

    scroll->setWidget(page);
    m_stack->addWidget(scroll);

    // ---- page 1: single shelf detail ----
    m_shelfDetail = new ShelfDetailPage(this);
    connect(m_shelfDetail, &ShelfDetailPage::backRequested, this, &MyLibraryPage::backToLibrary);
    connect(m_shelfDetail, &ShelfDetailPage::editRequested, this, &MyLibraryPage::openEditShelfDialog);
    connect(m_shelfDetail, &ShelfDetailPage::deleteRequested, this, [this](int shelfId) {
        emit deleteShelfRequested(shelfId);
        backToLibrary();
    });
    connect(m_shelfDetail, &ShelfDetailPage::bookOpenRequested, this, &MyLibraryPage::bookOpenRequested);
    connect(m_shelfDetail, &ShelfDetailPage::bookReadRequested, this, &MyLibraryPage::bookReadRequested);
    connect(m_shelfDetail, &ShelfDetailPage::bookMoveRequested, this, &MyLibraryPage::openMoveDialog);
    connect(m_shelfDetail, &ShelfDetailPage::bookFavoriteToggleRequested, this, &MyLibraryPage::bookFavoriteToggleRequested);
    connect(m_shelfDetail, &ShelfDetailPage::bookRemoveFromShelfRequested, this, [this](int bookId) {
        emit removeBookFromShelfRequested(bookId, m_shelfDetail->currentShelfId());
    });
    m_stack->addWidget(m_shelfDetail);

    outer->addWidget(m_stack);

    setActiveTab(0);
}

// --------------------------------------------------------------- UI builders

QWidget *MyLibraryPage::buildHeader()
{
    auto *header = new QWidget(this);
    header->setAttribute(Qt::WA_StyledBackground, true);
    header->setMinimumHeight(190);
    header->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:0.3,"
        " stop:0 #2A0F45, stop:0.55 #1B0E2C, stop:1 #0F0A16);"
        "border: 1px solid rgba(255,255,255,0.08);"
        "border-radius: 18px;");

    auto *v = new QVBoxLayout(header);
    v->setContentsMargins(28, 24, 28, 24);
    v->setSpacing(18);

    auto *topRow = new QHBoxLayout;
    topRow->setSpacing(16);

    auto *iconBubble = new QLabel(QStringLiteral("\U0001F4D6"), header);
    iconBubble->setFixedSize(52, 52);
    iconBubble->setAlignment(Qt::AlignCenter);
    iconBubble->setStyleSheet(
        "background-color: rgba(255,255,255,20); border-radius: 14px; font-size: 24px; border:none;");

    auto *titleCol = new QVBoxLayout;
    titleCol->setSpacing(2);
    auto *title = new QLabel("My Library", header);
    title->setStyleSheet("color:#FFFFFF; font-size:26px; font-weight:800; background:transparent; border:none;");
    auto *subtitle = new QLabel("Manage your personal collection", header);
    subtitle->setStyleSheet("color: rgba(255,255,255,150); font-size:13px; background:transparent; border:none;");
    titleCol->addWidget(title);
    titleCol->addWidget(subtitle);

    topRow->addWidget(iconBubble);
    topRow->addLayout(titleCol);
    topRow->addStretch();

    auto *decoration = new QLabel(QStringLiteral("\U0001F4DA"), header);
    decoration->setAttribute(Qt::WA_TransparentForMouseEvents);
    decoration->setStyleSheet("color: rgba(255,255,255,25); font-size:96px; background:transparent; border:none;");
    topRow->addWidget(decoration);

    v->addLayout(topRow);

    auto *statsRow = new QHBoxLayout;
    statsRow->setSpacing(14);
    m_statBooks     = new StatisticsCard(QStringLiteral("\U0001F4D8"), "0", "Books", header);
    m_statShelves   = new StatisticsCard(QStringLiteral("\U0001F5C2"),  "0", "Shelves", header);
    m_statReading   = new StatisticsCard(QStringLiteral("\u23F1"),      "0", "Currently Reading", header);
    m_statFavorites = new StatisticsCard(QStringLiteral("\u2B50"),      "0", "Favorites", header);
    statsRow->addWidget(m_statBooks);
    statsRow->addWidget(m_statShelves);
    statsRow->addWidget(m_statReading);
    statsRow->addWidget(m_statFavorites);
    v->addLayout(statsRow);

    return header;
}

QWidget *MyLibraryPage::buildTabsAndToolbar()
{
    auto *wrap = new QWidget(this);
    auto *v = new QVBoxLayout(wrap);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(16);

    // ---- pill tab bar ----
    auto *tabsHost = new QWidget(wrap);
    tabsHost->setAttribute(Qt::WA_StyledBackground, true);
    tabsHost->setFixedHeight(42);
    tabsHost->setStyleSheet("background-color:#181320; border-radius:12px; border:none;");
    auto *tabsRow = new QHBoxLayout(tabsHost);
    tabsRow->setContentsMargins(5, 5, 5, 5);
    tabsRow->setSpacing(4);

    auto makeTab = [tabsHost](const QString &text) {
        auto *b = new QPushButton(text, tabsHost);
        b->setCursor(Qt::PointingHandCursor);
        b->setCheckable(true);
        b->setFixedHeight(32);
        b->setStyleSheet(kTabButtonStyle);
        return b;
    };
    m_tabAll = makeTab("All");
    m_tabMyBooks = makeTab("My Books");
    m_tabShelves = makeTab("Shelves");

    auto *tabGroup = new QButtonGroup(this);
    tabGroup->setExclusive(true);
    tabGroup->addButton(m_tabAll);
    tabGroup->addButton(m_tabMyBooks);
    tabGroup->addButton(m_tabShelves);

    connect(m_tabAll, &QPushButton::clicked, this, [this] { setActiveTab(0); });
    connect(m_tabMyBooks, &QPushButton::clicked, this, [this] { setActiveTab(1); });
    connect(m_tabShelves, &QPushButton::clicked, this, [this] { setActiveTab(2); });

    tabsRow->addWidget(m_tabAll);
    tabsRow->addWidget(m_tabMyBooks);
    tabsRow->addWidget(m_tabShelves);
    tabsRow->addStretch();
    v->addWidget(tabsHost);

    // ---- toolbar: search / sort / filter / view toggle ----
    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(12);

    m_searchEdit = new QLineEdit(wrap);
    m_searchEdit->setPlaceholderText(QStringLiteral("\U0001F50D  Search my library..."));
    m_searchEdit->setFixedHeight(40);
    m_searchEdit->setStyleSheet(
        "QLineEdit{background-color:#181320; border:1px solid rgba(255,255,255,20);"
        "border-radius:12px; padding-left:14px; color:#EAEAEA; font-size:13px;}"
        "QLineEdit:focus{border-color:#A855F7;}");
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_searchText = text;
        applyFilters();
    });
    toolbar->addWidget(m_searchEdit, 1);

    auto makeCombo = [wrap](const QStringList &items) {
        auto *combo = new QComboBox(wrap);
        combo->addItems(items);
        combo->setFixedHeight(40);
        combo->setMinimumWidth(150);
        combo->setCursor(Qt::PointingHandCursor);
        combo->setStyleSheet(
            "QComboBox{background-color:#181320; border:1px solid rgba(255,255,255,20);"
            "border-radius:12px; padding-left:12px; color:#EAEAEA; font-size:13px;}"
            "QComboBox:hover{border-color:#A855F7;}"
            "QComboBox::drop-down{border:none; width:26px;}"
            "QComboBox QAbstractItemView{background-color:#181320; color:#EAEAEA;"
            "border:1px solid rgba(255,255,255,30); selection-background-color:#A855F7;"
            "outline:none;}");
        return combo;
    };

    m_sortCombo = makeCombo({"Newest", "Title A-Z", "Author A-Z", "Highest Rated"});
    connect(m_sortCombo, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        m_sortMode = text;
        applyFilters();
    });
    toolbar->addWidget(m_sortCombo);

    m_filterCombo = makeCombo({"All"});
    connect(m_filterCombo, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        m_filterGenre = text;
        applyFilters();
    });
    toolbar->addWidget(m_filterCombo);

    auto *viewToggle = new QWidget(wrap);
    viewToggle->setAttribute(Qt::WA_StyledBackground, true);
    viewToggle->setFixedHeight(40);
    viewToggle->setStyleSheet(
        "background-color:#181320; border:1px solid rgba(255,255,255,20); border-radius:12px;");
    auto *viewRow = new QHBoxLayout(viewToggle);
    viewRow->setContentsMargins(4, 4, 4, 4);
    viewRow->setSpacing(2);

    m_viewGridBtn = new QPushButton(QStringLiteral("\u2637"), viewToggle);
    m_viewListBtn = new QPushButton(QStringLiteral("\u2630"), viewToggle);
    auto *viewGroup = new QButtonGroup(this);
    viewGroup->setExclusive(true);
    for (QPushButton *b : {m_viewGridBtn, m_viewListBtn}) {
        b->setCheckable(true);
        b->setFixedSize(32, 32);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(kViewToggleStyle);
        viewGroup->addButton(b);
    }
    m_viewGridBtn->setChecked(true);
    connect(m_viewGridBtn, &QPushButton::clicked, this, [this] { m_gridView = true; relayoutBooksGrid(); });
    connect(m_viewListBtn, &QPushButton::clicked, this, [this] { m_gridView = false; relayoutBooksGrid(); });
    viewRow->addWidget(m_viewGridBtn);
    viewRow->addWidget(m_viewListBtn);
    toolbar->addWidget(viewToggle);

    v->addLayout(toolbar);
    return wrap;
}

QWidget *MyLibraryPage::buildContinueReadingSection()
{
    auto *wrap = new QWidget(this);
    auto *v = new QVBoxLayout(wrap);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(12);

    auto *headRow = new QHBoxLayout;
    auto *title = new QLabel("Continue Reading", wrap);
    title->setStyleSheet("color:#FFFFFF; font-size:17px; font-weight:700; background:transparent; border:none;");
    auto *seeAll = new QPushButton(QStringLiteral("See All \u2192"), wrap);
    seeAll->setCursor(Qt::PointingHandCursor);
    seeAll->setStyleSheet(
        "QPushButton{background:transparent;border:none;color:#A855F7;font-size:12px;font-weight:600;}"
        "QPushButton:hover{color:#C084FC;}");
    connect(seeAll, &QPushButton::clicked, this, [this] { setActiveTab(1); });
    headRow->addWidget(title);
    headRow->addStretch();
    headRow->addWidget(seeAll);
    v->addLayout(headRow);

    auto *scroll = new QScrollArea(wrap);
    scroll->setWidgetResizable(false);
    scroll->setFixedHeight(128);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(
        "QScrollArea{border:none;background:transparent;}"
        "QScrollBar:horizontal{height:8px;background:#181320;border-radius:4px;}"
        "QScrollBar::handle:horizontal{background:#A855F7;border-radius:4px;min-width:40px;}"
        "QScrollBar::handle:horizontal:hover{background:#C084FC;}");

    m_continueRowHost = new QWidget;
    m_continueRowHost->setStyleSheet("background:transparent;");
    m_continueRow = new QHBoxLayout(m_continueRowHost);
    m_continueRow->setContentsMargins(0, 0, 0, 0);
    m_continueRow->setSpacing(14);
    m_continueRow->addStretch();

    scroll->setWidget(m_continueRowHost);
    v->addWidget(scroll);

    return wrap;
}

QWidget *MyLibraryPage::buildMyBooksSection()
{
    auto *wrap = new QWidget(this);
    auto *v = new QVBoxLayout(wrap);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(12);

    auto *headRow = new QHBoxLayout;
    auto *title = new QLabel("My Books", wrap);
    title->setStyleSheet("color:#FFFFFF; font-size:17px; font-weight:700; background:transparent; border:none;");
    m_booksSeeAll = new QPushButton(QStringLiteral("See All \u2192"), wrap);
    m_booksSeeAll->setCursor(Qt::PointingHandCursor);
    m_booksSeeAll->setStyleSheet(
        "QPushButton{background:transparent;border:none;color:#A855F7;font-size:12px;font-weight:600;}"
        "QPushButton:hover{color:#C084FC;}");
    connect(m_booksSeeAll, &QPushButton::clicked, this, [this] { setActiveTab(1); });
    headRow->addWidget(title);
    headRow->addStretch();
    headRow->addWidget(m_booksSeeAll);
    v->addLayout(headRow);

    m_booksGridHost = new QWidget(wrap);
    m_booksGridHost->setStyleSheet("background:transparent;");
    m_booksGrid = new QGridLayout(m_booksGridHost);
    m_booksGrid->setSpacing(18);
    m_booksGrid->setContentsMargins(0, 0, 0, 0);
    m_booksGrid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    v->addWidget(m_booksGridHost);

    return wrap;
}

QWidget *MyLibraryPage::buildShelvesSection()
{
    auto *wrap = new QWidget(this);
    auto *v = new QVBoxLayout(wrap);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(12);

    auto *title = new QLabel("My Shelves", wrap);
    title->setStyleSheet("color:#FFFFFF; font-size:17px; font-weight:700; background:transparent; border:none;");
    v->addWidget(title);

    m_shelvesGridHost = new QWidget(wrap);
    m_shelvesGridHost->setStyleSheet("background:transparent;");
    m_shelvesGrid = new QGridLayout(m_shelvesGridHost);
    m_shelvesGrid->setSpacing(18);
    m_shelvesGrid->setContentsMargins(0, 0, 0, 0);
    m_shelvesGrid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    v->addWidget(m_shelvesGridHost);

    return wrap;
}

// ------------------------------------------------------------------ setters

void MyLibraryPage::setStatistics(int totalBooks, int totalShelves, int currentlyReading, int favorites)
{
    m_statBooks->setValue(QString::number(totalBooks));
    m_statShelves->setValue(QString::number(totalShelves));
    m_statReading->setValue(QString::number(currentlyReading));
    m_statFavorites->setValue(QString::number(favorites));
}

void MyLibraryPage::setContinueReading(const QVector<ContinueReadingItem> &items)
{
    m_continueReading = items;
    rebuildContinueReadingRow();
    updateSectionVisibility();
}

void MyLibraryPage::setMyBooks(const QVector<Book> &books)
{
    m_allBooks = books;

    const QString keepFilter = m_filterCombo->currentText();
    m_filterCombo->blockSignals(true);
    m_filterCombo->clear();
    m_filterCombo->addItem("All");

    QSet<QString> genres;
    for (const Book &b : m_allBooks)
        if (!b.genre.trimmed().isEmpty())
            genres.insert(b.genre);
    QStringList sortedGenres(genres.begin(), genres.end());
    std::sort(sortedGenres.begin(), sortedGenres.end());
    m_filterCombo->addItems(sortedGenres);

    const int idx = m_filterCombo->findText(keepFilter);
    m_filterCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    m_filterGenre = m_filterCombo->currentText();
    m_filterCombo->blockSignals(false);

    applyFilters();
}

void MyLibraryPage::setShelves(const QVector<ShelfSummary> &shelves)
{
    m_shelves = shelves;
    rebuildShelvesGrid();
}

void MyLibraryPage::setFavoriteBookIds(const QSet<int> &bookIds)
{
    m_favoriteIds = bookIds;
    for (BookCard *card : m_bookCards)
        card->setFavorite(m_favoriteIds.contains(card->bookId()));
}

void MyLibraryPage::setCoverProvider(const CoverProvider &provider)
{
    m_coverProvider = provider;
    m_shelfDetail->setCoverProvider(provider);

    // Re-render whatever is already on screen so calling this after the
    // data setters (or before) both work.
    rebuildContinueReadingRow();
    rebuildBooksGrid();
    rebuildShelvesGrid();
}

void MyLibraryPage::showShelfDetail(const ShelfSummary &summary, const QVector<Book> &books)
{
    m_shelfDetail->setShelf(summary, books);
    m_stack->setCurrentIndex(1);
}

void MyLibraryPage::backToLibrary()
{
    m_stack->setCurrentIndex(0);
}

// -------------------------------------------------------------------- tabs

void MyLibraryPage::setActiveTab(int index)
{
    m_activeTab = index;
    m_tabAll->setChecked(index == 0);
    m_tabMyBooks->setChecked(index == 1);
    m_tabShelves->setChecked(index == 2);
    updateSectionVisibility();
}

void MyLibraryPage::updateSectionVisibility()
{
    const bool showAll = m_activeTab == 0;
    const bool showBooks = m_activeTab == 0 || m_activeTab == 1;
    const bool showShelves = m_activeTab == 0 || m_activeTab == 2;

    m_continueSection->setVisible(showAll && !m_continueReading.isEmpty());
    m_booksSection->setVisible(showBooks);
    m_booksSeeAll->setVisible(showAll);
    m_shelvesSection->setVisible(showShelves);
}

// ----------------------------------------------------------------- filters

void MyLibraryPage::applyFilters()
{
    QVector<Book> result;
    const QString needle = m_searchText.trimmed();

    for (const Book &b : m_allBooks) {
        if (m_filterGenre != "All" && b.genre.compare(m_filterGenre, Qt::CaseInsensitive) != 0)
            continue;
        if (!needle.isEmpty() &&
            !b.title.contains(needle, Qt::CaseInsensitive) &&
            !b.author.contains(needle, Qt::CaseInsensitive))
            continue;
        result.push_back(b);
    }

    if (m_sortMode == "Title A-Z") {
        std::sort(result.begin(), result.end(),
                  [](const Book &a, const Book &b) { return a.title.localeAwareCompare(b.title) < 0; });
    } else if (m_sortMode == "Author A-Z") {
        std::sort(result.begin(), result.end(),
                  [](const Book &a, const Book &b) { return a.author.localeAwareCompare(b.author) < 0; });
    } else if (m_sortMode == "Highest Rated") {
        std::sort(result.begin(), result.end(),
                  [](const Book &a, const Book &b) { return a.averageRating > b.averageRating; });
    } else { // "Newest"
        std::sort(result.begin(), result.end(),
                  [](const Book &a, const Book &b) { return a.id > b.id; });
    }

    m_filteredBooks = result;
    rebuildBooksGrid();
}

// -------------------------------------------------------------- rebuilders

void MyLibraryPage::rebuildContinueReadingRow()
{
    while (m_continueRow->count() > 0) {
        QLayoutItem *item = m_continueRow->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    for (const ContinueReadingItem &item : m_continueReading) {
        auto *card = new ContinueReadingCard(item, m_continueRowHost);
        if (m_coverProvider) {
            QPixmap pix = m_coverProvider(item.book, QSize(60, 84));
            if (!pix.isNull()) card->setCoverPixmap(pix);
        }
        connect(card, &ContinueReadingCard::continueRequested, this, &MyLibraryPage::bookReadRequested);
        m_continueRow->addWidget(card);
    }
    m_continueRow->addStretch();
}

void MyLibraryPage::rebuildBooksGrid()
{
    for (BookCard *card : m_bookCards)
        card->deleteLater();
    m_bookCards.clear();

    for (const Book &b : m_filteredBooks) {
        auto *card = new BookCard(b, m_booksGridHost);
        card->setFavorite(m_favoriteIds.contains(b.id));
        if (m_coverProvider) {
            QPixmap pix = m_coverProvider(b, QSize(144, 190));
            if (!pix.isNull()) card->setCoverPixmap(pix);
        }
        connect(card, &BookCard::openRequested, this, &MyLibraryPage::bookOpenRequested);
        connect(card, &BookCard::readRequested, this, &MyLibraryPage::bookReadRequested);
        connect(card, &BookCard::moveToShelfRequested, this, &MyLibraryPage::openMoveDialog);
        connect(card, &BookCard::favoriteToggleRequested, this, &MyLibraryPage::bookFavoriteToggleRequested);
        m_bookCards.append(card);
    }
    relayoutBooksGrid();
}

void MyLibraryPage::relayoutBooksGrid()
{
    while (m_booksGrid->count() > 0)
        m_booksGrid->takeAt(0);

    const int cardWidth = 168;
    const int spacing = 18;
    int columns = 1;
    if (m_gridView) {
        const int available = m_booksGridHost->width() > 0 ? m_booksGridHost->width() : width();
        columns = qMax(1, (available + spacing) / (cardWidth + spacing));
    }

    int row = 0, col = 0;
    for (BookCard *card : m_bookCards) {
        m_booksGrid->addWidget(card, row, col);
        if (++col >= columns) { col = 0; ++row; }
    }
}

void MyLibraryPage::rebuildShelvesGrid()
{
    for (ShelfCard *card : m_shelfCards)
        card->deleteLater();
    m_shelfCards.clear();

    if (m_addShelfCard) {
        m_addShelfCard->deleteLater();
        m_addShelfCard = nullptr;
    }

    for (const ShelfSummary &s : m_shelves) {
        auto *card = new ShelfCard(s, m_coverProvider, m_shelvesGridHost);
        connect(card, &ShelfCard::openRequested, this, &MyLibraryPage::openShelfDetail);
        connect(card, &ShelfCard::editRequested, this, &MyLibraryPage::openEditShelfDialog);
        connect(card, &ShelfCard::deleteRequested, this, &MyLibraryPage::deleteShelfRequested);
        m_shelfCards.append(card);
    }

    m_addShelfCard = new AddShelfCard(m_shelvesGridHost);
    connect(m_addShelfCard, &AddShelfCard::createRequested, this, &MyLibraryPage::openCreateShelfDialog);

    relayoutShelvesGrid();
}

void MyLibraryPage::relayoutShelvesGrid()
{
    while (m_shelvesGrid->count() > 0)
        m_shelvesGrid->takeAt(0);

    const int cardWidth = 176;
    const int spacing = 18;
    const int available = m_shelvesGridHost->width() > 0 ? m_shelvesGridHost->width() : width();
    const int columns = qMax(1, (available + spacing) / (cardWidth + spacing));

    int row = 0, col = 0;
    for (ShelfCard *card : m_shelfCards) {
        m_shelvesGrid->addWidget(card, row, col);
        if (++col >= columns) { col = 0; ++row; }
    }
    if (m_addShelfCard)
        m_shelvesGrid->addWidget(m_addShelfCard, row, col);
}

// --------------------------------------------------------------- dialogs

void MyLibraryPage::openMoveDialog(int bookId)
{
    if (m_shelves.isEmpty()) {
        openCreateShelfDialog();
        return;
    }
    MoveToShelfDialog dialog(m_shelves, -1, this);
    connect(&dialog, &MoveToShelfDialog::createNewShelfRequested, this, &MyLibraryPage::openCreateShelfDialog);
    if (dialog.exec() == QDialog::Accepted && dialog.selectedShelfId() != -1)
        emit moveBookToShelfRequested(bookId, dialog.selectedShelfId());
}

void MyLibraryPage::openCreateShelfDialog()
{
    CreateShelfDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted && !dialog.shelfName().isEmpty())
        emit createShelfRequested(dialog.shelfName(), dialog.shelfDescription(), dialog.selectedColor());
}

void MyLibraryPage::openEditShelfDialog(int shelfId)
{
    QString name;
    QColor color("#A855F7");
    for (const ShelfSummary &s : m_shelves) {
        if (s.shelf.id == shelfId) {
            name = s.shelf.title;
            color = shelfColorForTitle(name);
            break;
        }
    }

    CreateShelfDialog dialog(this, true, name, QString(), color);
    if (dialog.exec() == QDialog::Accepted && !dialog.shelfName().isEmpty())
        emit editShelfRequested(shelfId, dialog.shelfName(), dialog.shelfDescription(), dialog.selectedColor());
}

void MyLibraryPage::openShelfDetail(int shelfId)
{
    emit shelfOpened(shelfId);
}

// ------------------------------------------------------------------ resize

void MyLibraryPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayoutBooksGrid();
    relayoutShelvesGrid();
}

int MyLibraryPage::currentlyViewedShelfId() const
{
    return m_stack->currentIndex() == 1 ? m_shelfDetail->currentShelfId() : -1;
}
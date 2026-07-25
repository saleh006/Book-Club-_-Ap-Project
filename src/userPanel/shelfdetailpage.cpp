#include "shelfdetailpage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QResizeEvent>

ShelfDetailPage::ShelfDetailPage(QWidget *parent) : QWidget(parent)
{
    setStyleSheet("background-color:#09070D;");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 20, 30, 30);
    root->setSpacing(20);

    auto *topBar = new QHBoxLayout;
    topBar->setSpacing(14);

    auto *backBtn = new QPushButton(QStringLiteral("\u2190"), this);
    backBtn->setFixedSize(38, 38);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet(
        "QPushButton{background-color:#181320;border:1px solid rgba(255,255,255,20);"
        "border-radius:10px;color:#EAEAEA;font-size:15px;}"
        "QPushButton:hover{background-color:#A855F7;border-color:#C084FC;color:white;}");
    connect(backBtn, &QPushButton::clicked, this, &ShelfDetailPage::backRequested);
    topBar->addWidget(backBtn);

    m_iconLbl = new QLabel(this);
    m_iconLbl->setFixedSize(38, 38);
    m_iconLbl->setAlignment(Qt::AlignCenter);
    m_iconLbl->setStyleSheet("background-color:#181320; border-radius:10px; font-size:16px;");
    topBar->addWidget(m_iconLbl);

    auto *nameCol = new QVBoxLayout;
    nameCol->setSpacing(2);
    m_nameLbl = new QLabel(this);
    m_nameLbl->setStyleSheet("color:#FFFFFF; font-size:20px; font-weight:700; background:transparent; border:none;");
    m_countLbl = new QLabel(this);
    m_countLbl->setStyleSheet("color: rgba(255,255,255,140); font-size:12px; background:transparent; border:none;");
    nameCol->addWidget(m_nameLbl);
    nameCol->addWidget(m_countLbl);
    topBar->addLayout(nameCol);

    topBar->addStretch();

    auto *editBtn = new QPushButton("Edit Shelf", this);
    editBtn->setFixedHeight(36);
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setStyleSheet(
        "QPushButton{background-color:#A855F7;border:none;border-radius:9px;"
        "color:white;font-size:12px;font-weight:700;padding:0 16px;}"
        "QPushButton:hover{background-color:#C084FC;}");
    connect(editBtn, &QPushButton::clicked, this, [this] { emit editRequested(m_summary.shelf.id); });
    topBar->addWidget(editBtn);

    auto *deleteBtn = new QPushButton("Delete Shelf", this);
    deleteBtn->setFixedHeight(36);
    deleteBtn->setCursor(Qt::PointingHandCursor);
    deleteBtn->setStyleSheet(
        "QPushButton{background:transparent;border:1px solid rgba(239,68,68,140);border-radius:9px;"
        "color:#EF4444;font-size:12px;font-weight:700;padding:0 16px;}"
        "QPushButton:hover{background-color:rgba(239,68,68,30);}");
    connect(deleteBtn, &QPushButton::clicked, this, [this] { emit deleteRequested(m_summary.shelf.id); });
    topBar->addWidget(deleteBtn);

    root->addLayout(topBar);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(
        "QScrollArea{border:none;background:transparent;}"
        "QScrollBar:vertical{width:8px;background:#120E14;border-radius:4px;}"
        "QScrollBar::handle:vertical{background:#A855F7;border-radius:4px;min-height:30px;}"
        "QScrollBar::handle:vertical:hover{background:#C084FC;}");

    m_gridHost = new QWidget;
    m_gridHost->setStyleSheet("background:transparent;");
    m_grid = new QGridLayout(m_gridHost);
    m_grid->setSpacing(18);
    m_grid->setContentsMargins(0, 0, 0, 0);
    m_grid->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    scroll->setWidget(m_gridHost);
    root->addWidget(scroll, 1);
}

void ShelfDetailPage::setShelf(const ShelfSummary &summary, const QVector<Book> &books)
{
    m_summary = summary;
    m_books = books;
    m_iconLbl->setText(shelfIconForTitle(summary.shelf.title));
    m_nameLbl->setText(summary.shelf.title);
    m_countLbl->setText(QString("%1 Books").arg(books.size()));
    rebuildGrid();
}

void ShelfDetailPage::setCoverProvider(const CoverProvider &provider)
{
    m_coverProvider = provider;
    rebuildGrid();
}

void ShelfDetailPage::rebuildGrid()
{
    for (BookCard *card : m_cards)
        card->deleteLater();
    m_cards.clear();

    for (const Book &b : m_books) {
        auto *card = new BookCard(b, m_gridHost);
        if (m_coverProvider) {
            QPixmap pix = m_coverProvider(b, QSize(144, 190));
            if (!pix.isNull()) card->setCoverPixmap(pix);
        }
        connect(card, &BookCard::openRequested, this, &ShelfDetailPage::bookOpenRequested);
        connect(card, &BookCard::readRequested, this, &ShelfDetailPage::bookReadRequested);
        connect(card, &BookCard::moveToShelfRequested, this, &ShelfDetailPage::bookMoveRequested);
        connect(card, &BookCard::favoriteToggleRequested, this, &ShelfDetailPage::bookFavoriteToggleRequested);
        m_cards.append(card);
    }
    relayoutGrid();
}

void ShelfDetailPage::relayoutGrid()
{
    while (m_grid->count() > 0)
        m_grid->takeAt(0);

    const int cardWidth = 168;
    const int spacing = 18;
    const int available = m_gridHost->width() > 0 ? m_gridHost->width() : width();
    const int columns = qMax(1, (available + spacing) / (cardWidth + spacing));

    int row = 0, col = 0;
    for (BookCard *card : m_cards) {
        m_grid->addWidget(card, row, col);
        if (++col >= columns) { col = 0; ++row; }
    }
}

void ShelfDetailPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayoutGrid();
}
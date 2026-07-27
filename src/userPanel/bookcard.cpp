#include "bookcard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>

namespace {

QPixmap placeholderCover(const Book &b, const QSize &size)
{
    static const QStringList colors = {"#3B2A4D", "#4D2A3E", "#2A3E4D", "#2A4D3B", "#4D3B2A"};
    QPixmap canvas(size);
    canvas.fill(QColor(colors[qAbs(b.id) % colors.size()]));

    QPainter p(&canvas);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QColor("#EAEAEA"));
    QFont f = p.font();
    f.setBold(true);
    f.setPointSize(10);
    p.setFont(f);
    p.drawText(canvas.rect().adjusted(10, 10, -10, -10), Qt::AlignCenter | Qt::TextWordWrap, b.title);
    p.end();
    return canvas;
}

} // namespace

// ---------------------------------------------------------------- BookCard

BookCard::BookCard(const Book &book, QWidget *parent)
    : HoverCard(parent), m_book(book)
{
    setBackgroundColor(QColor("#181320"));
    setBorderColors(QColor(255, 255, 255, 20), QColor("#A855F7"));
    setCornerRadius(18);
    setFixedWidth(168);

    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(12, 12, 12, 12);
    v->setSpacing(6);

    m_coverLabel = new QLabel(this);
    m_coverLabel->setFixedSize(144, 190);
    m_coverLabel->setScaledContents(true);
    m_coverLabel->setPixmap(placeholderCover(m_book, QSize(144, 190)));
    m_coverLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_coverLabel->setStyleSheet("border-radius: 10px; background: transparent;");
    v->addWidget(m_coverLabel, 0, Qt::AlignHCenter);

    auto *titleLbl = new QLabel(m_book.title, this);
    titleLbl->setWordWrap(true);
    titleLbl->setMaximumHeight(38);
    titleLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
    titleLbl->setStyleSheet(
        "color:#FFFFFF; font-size:13px; font-weight:600; background:transparent; border:none;");
    v->addWidget(titleLbl);

    auto *authorLbl = new QLabel(m_book.author, this);
    authorLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
    authorLbl->setStyleSheet(
        "color: rgba(255,255,255,140); font-size:11px; background:transparent; border:none;");
    v->addWidget(authorLbl);

    auto *ratingLbl = new QLabel(QString("\u2605 %1").arg(m_book.averageRating, 0, 'f', 1), this);
    ratingLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
    ratingLbl->setStyleSheet(
        "color:#F0B429; font-size:12px; font-weight:600; background:transparent; border:none;");
    v->addWidget(ratingLbl);

    v->addStretch();

    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(6);

    auto *readBtn = makeIconButton(":/icons/link.png",
                                   QStringLiteral("Read"));
    connect(readBtn, &QPushButton::clicked, this, [this] { emit readRequested(m_book.id); });

    auto *shelfBtn = makeIconButton(":/icons/folder.png",
                                    QStringLiteral("Move to shelf"));
    connect(shelfBtn, &QPushButton::clicked, this, [this] { emit moveToShelfRequested(m_book.id); });

    m_favoriteBtn = makeIconButton(QStringLiteral("\u2661"), QStringLiteral("Favorite"));
    connect(m_favoriteBtn, &QPushButton::clicked, this, [this] { emit favoriteToggleRequested(m_book.id); });

    auto *moreBtn = makeIconButton(QStringLiteral("\u22EF"), QStringLiteral("More"));
    connect(moreBtn, &QPushButton::clicked, this, [this] { emit moreOptionsRequested(m_book.id); });

    btnRow->addWidget(readBtn);
    btnRow->addWidget(shelfBtn);
    btnRow->addWidget(m_favoriteBtn);
    btnRow->addWidget(moreBtn);
    v->addLayout(btnRow);

    connect(this, &HoverCard::clicked, this, [this] { emit openRequested(m_book.id); });
}

QPushButton *BookCard::makeIconButton(const QString &iconPath,
                                      const QString &tooltip)
{
    auto *btn = new QPushButton(this);

    btn->setFixedSize(30, 28);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setToolTip(tooltip);

    if (iconPath.startsWith(":/"))
    {
        btn->setIcon(QIcon(iconPath));
        btn->setIconSize(QSize(16,16));
    }
    else
    {
        btn->setText(iconPath);
    }

    btn->setStyleSheet(
        "QPushButton{background-color:#231B2F;"
        "border:1px solid rgba(255,255,255,20);"
        "border-radius:8px;color:#EAEAEA;font-size:12px;}"
        "QPushButton:hover{background-color:#A855F7;"
        "border-color:#C084FC;color:white;}"
        );

    return btn;
}

void BookCard::setFavorite(bool favorite)
{
    m_favorite = favorite;
    m_favoriteBtn->setText(m_favorite ? QStringLiteral("\u2665") : QStringLiteral("\u2661"));
    m_favoriteBtn->setStyleSheet(m_favorite
                                     ? "QPushButton{background-color:#A855F7;border:1px solid #C084FC;"
                                       "border-radius:8px;color:white;font-size:12px;}"
                                       "QPushButton:hover{background-color:#C084FC;}"
                                     : "QPushButton{background-color:#231B2F;border:1px solid rgba(255,255,255,20);"
                                       "border-radius:8px;color:#EAEAEA;font-size:12px;}"
                                       "QPushButton:hover{background-color:#A855F7;border-color:#C084FC;color:white;}");
}

void BookCard::setCoverPixmap(const QPixmap &pixmap)
{
    if (!pixmap.isNull())
        m_coverLabel->setPixmap(pixmap);
}

// ------------------------------------------------------- ContinueReadingCard

ContinueReadingCard::ContinueReadingCard(const ContinueReadingItem &item, QWidget *parent)
    : HoverCard(parent), m_item(item)
{
    setBackgroundColor(QColor("#181320"));
    setBorderColors(QColor(255, 255, 255, 20), QColor("#A855F7"));
    setCornerRadius(16);
    setFixedSize(260, 108);

    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(10, 10, 10, 10);
    row->setSpacing(10);

    m_coverLabel = new QLabel(this);
    m_coverLabel->setFixedSize(60, 84);
    m_coverLabel->setScaledContents(true);
    m_coverLabel->setPixmap(placeholderCover(m_item.book, QSize(60, 84)));
    m_coverLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_coverLabel->setStyleSheet("border-radius:8px;");
    row->addWidget(m_coverLabel);

    auto *col = new QVBoxLayout;
    col->setSpacing(4);

    QFontMetrics fm(font());
    auto *titleLbl = new QLabel(fm.elidedText(m_item.book.title, Qt::ElideRight, 130), this);
    titleLbl->setToolTip(m_item.book.title);
    titleLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
    titleLbl->setStyleSheet(
        "color:#FFFFFF; font-size:12px; font-weight:600; background:transparent; border:none;");

    auto *authorLbl = new QLabel(fm.elidedText(m_item.book.author, Qt::ElideRight, 130), this);
    authorLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
    authorLbl->setStyleSheet(
        "color: rgba(255,255,255,140); font-size:10px; background:transparent; border:none;");

    auto *progressWrap = new QWidget(this);
    progressWrap->setFixedHeight(6);
    progressWrap->setAttribute(Qt::WA_TransparentForMouseEvents);
    progressWrap->setStyleSheet("background-color: rgba(255,255,255,25); border-radius: 3px;");
    auto *progressLayout = new QHBoxLayout(progressWrap);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    progressLayout->setSpacing(0);

    const int pct = qBound(0, m_item.progressPercent, 100);
    auto *fill = new QWidget(progressWrap);
    fill->setStyleSheet("background-color:#A855F7; border-radius:3px;");
    progressLayout->addWidget(fill, qMax(pct, 1));
    if (pct < 100) {
        auto *spacer = new QWidget(progressWrap);
        spacer->setStyleSheet("background: transparent;");
        progressLayout->addWidget(spacer, 100 - pct);
    }

    auto *pctLbl = new QLabel(QString("%1%").arg(pct), this);
    pctLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
    pctLbl->setStyleSheet(
        "color: rgba(255,255,255,150); font-size:10px; background:transparent; border:none;");

    col->addWidget(titleLbl);
    col->addWidget(authorLbl);
    col->addStretch();
    col->addWidget(progressWrap);
    col->addWidget(pctLbl);

    row->addLayout(col, 1);

    auto *playBtn = new QPushButton(QStringLiteral("\u25B6"), this);
    playBtn->setFixedSize(34, 34);
    playBtn->setCursor(Qt::PointingHandCursor);
    playBtn->setStyleSheet(
        "QPushButton{background-color:#A855F7;border:none;border-radius:17px;color:white;font-size:13px;}"
        "QPushButton:hover{background-color:#C084FC;}");
    connect(playBtn, &QPushButton::clicked, this, [this] { emit continueRequested(m_item.book.id); });
    row->addWidget(playBtn, 0, Qt::AlignVCenter);

    connect(this, &HoverCard::clicked, this, [this] { emit continueRequested(m_item.book.id); });
}

void ContinueReadingCard::setCoverPixmap(const QPixmap &pixmap)
{
    if (!pixmap.isNull())
        m_coverLabel->setPixmap(pixmap);
}

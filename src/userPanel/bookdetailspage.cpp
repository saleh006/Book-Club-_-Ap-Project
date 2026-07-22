#include "bookdetailspage.h"
#include <QScrollArea>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QFrame>
#include <QGraphicsDropShadowEffect>

// ---- Design tokens -------------------------------------------------------
// A single palette + scale so every card/button reads as one system instead
// of four separately-styled widgets. Gold is reserved for "value" signals
// (price, rating) so it never collides with green, which means only one
// thing: "you own this."
static const char *kBg          = "#07050C";
static const char *kSurface     = "#140F1B";   // card background
static const char *kSurfaceAlt  = "#0B0810";   // nested surfaces (input, review rows)
static const char *kBorder      = "#241A2E";
static const char *kAccent      = "#8B4C74";
static const char *kAccentHover = "#A6628C";
static const char *kGold        = "#E8B84B";   // price, star rating
static const char *kGreen       = "#3FAE72";   // owned / open-book state only
static const char *kGreenHover  = "#55C687";
static const char *kPink        = "#E0709E";   // wishlist
static const char *kTextPrimary = "#F5F1F7";
static const char *kTextDim     = "#A79AB0";
static const char *kTextFaint   = "#665A72";
static const char *kDisplayFont = "'Georgia','Iowan Old Style','Times New Roman',serif";

static const int kRadiusCard = 16;
static const int kRadiusBtn  = 10;

// same placeholder logic as UserPanel — consider moving to a shared coverutils.h later
static QPixmap roundedPixmap(const QPixmap &src, int radius)
{
    QPixmap out(src.size());
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(out.rect(), radius, radius);
    p.setClipPath(path);
    p.drawPixmap(0, 0, src);
    p.end();
    return out;
}

static QPixmap coverPixmap(const Book &b, const QSize &size)
{
    QPixmap base;
    QPixmap img(b.coverImagePath);
    if (!img.isNull()) {
        base = img.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)
        .copy(0, 0, size.width(), size.height());
    } else {
        base = QPixmap(size);
        static const QStringList colors = {"#3B2A4D", "#4D2A3E", "#2A3E4D", "#2A4D3B", "#4D3B2A"};
        base.fill(QColor(colors[qAbs(b.id) % colors.size()]));
        QPainter p(&base);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QColor(kTextPrimary));
        QFont f(kDisplayFont); f.setBold(true); f.setPointSize(11); p.setFont(f);
        p.drawText(base.rect().adjusted(10, 10, -10, -10), Qt::AlignCenter | Qt::TextWordWrap, b.title);
        p.end();
    }
    // Qt's stylesheet border-radius does NOT clip a QLabel's pixmap content,
    // so the rounding has to be baked into the pixmap itself.
    return roundedPixmap(base, 12);
}

static QString starGlyphs(double rating)
{
    int filled = qBound(0, qRound(rating), 5);
    return QString("★").repeated(filled) + QString("☆").repeated(5 - filled);
}

static void applyElevation(QWidget *w, qreal blur, qreal yOffset, int alpha)
{
    auto *shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(blur);
    shadow->setOffset(0, yOffset);
    shadow->setColor(QColor(0, 0, 0, alpha));
    w->setGraphicsEffect(shadow);
}

// hero = true gives the card a raised look and a signature accent ribbon
// on the left edge, so the top card reads as the primary object on the
// page and the rest read as supporting detail.
static void styleCard(QWidget *card, bool hero)
{
    QString qss = QString(
                      "background-color:%1;border:1px solid %2;border-radius:%3px;")
                      .arg(kSurface, kBorder).arg(kRadiusCard);
    if (hero) {
        qss += QString("border-left:4px solid %1;").arg(kAccent);
        qss += "background-image:url(qrc:/bookdhero.png);"
               "background-position:center;background-repeat:no-repeat;";
    }
    card->setStyleSheet(qss);
    applyElevation(card, hero ? 36 : 20, hero ? 10 : 5, hero ? 140 : 90);
}

static QString sectionHeadQss()
{
    return QString(
               "color:%1;font-size:15px;font-weight:600;letter-spacing:0.3px;"
               "border:none;border-bottom:1px solid %2;padding-bottom:10px;background:transparent;")
        .arg(kTextPrimary, kBorder);
}

static QString primaryButtonQss(const QString &top, const QString &bottom,
                                const QString &hoverTop, const QString &hoverBottom)
{
    return QString(
               "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %1, stop:1 %2);"
               "border:none;border-radius:%5px;padding:12px 24px;"
               "color:white;font-size:14px;font-weight:600;}"
               "QPushButton:hover{background:qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %3, stop:1 %4);}")
        .arg(top, bottom, hoverTop, hoverBottom).arg(kRadiusBtn);
}

BookDetailsPage::BookDetailsPage(QWidget *parent)
    : QWidget(parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QString("QScrollArea{border:none;background:%1;}").arg(kBg));

    auto *page = new QWidget;
    page->setStyleSheet(QString("background:%1;").arg(kBg));
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 24, 30, 32);
    layout->setSpacing(22);

    auto *backBtn = new QPushButton("←  Back", page);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setFixedWidth(100);
    backBtn->setStyleSheet(QString(
                               "QPushButton{background-color:%1;border:1px solid %2;border-radius:%3px;"
                               "padding:8px 14px;color:%4;font-size:13px;}"
                               "QPushButton:hover{border-color:%5;color:%5;}")
                               .arg(kSurface, kBorder).arg(kRadiusBtn).arg(kTextPrimary, kAccentHover));
    connect(backBtn, &QPushButton::clicked, this, &BookDetailsPage::backRequested);
    layout->addWidget(backBtn);

    layout->addWidget(buildTopCard());
    layout->addWidget(buildDescriptionCard());
    layout->addWidget(buildWriteReviewCard());
    layout->addWidget(buildReviewsCard());
    layout->addStretch();

    scroll->setWidget(page);
    outer->addWidget(scroll);
}

QWidget *BookDetailsPage::buildTopCard()
{
    auto *card = new HeroCard(this);
    card->setBackgroundImage(QPixmap(":/bookdhero.png"));
    applyElevation(card, 36, 10, 140);
    // don't call styleCard(card, true) here — HeroCard paints its own
    // background, border, and rounded corners in paintEvent now
    auto *h = new QHBoxLayout(card);
    h->setContentsMargins(26, 26, 26, 26);
    h->setSpacing(30);

    m_cover = new QLabel(card);
    m_cover->setFixedSize(200, 290);
    m_cover->setScaledContents(true);
    m_cover->setStyleSheet("border:none;background:transparent;");
    applyElevation(m_cover, 24, 8, 150);

    auto *info = new QVBoxLayout;
    info->setSpacing(8);

    m_genre = new QLabel(card);
    m_genre->setStyleSheet(QString(
                               "background-color:%1;color:white;border:none;border-radius:12px;"
                               "padding:5px 14px;font-size:11px;font-weight:bold;letter-spacing:0.4px;").arg(kAccent));
    m_genre->setFixedHeight(26);
    auto *genreWrap = new QHBoxLayout; genreWrap->addWidget(m_genre); genreWrap->addStretch();

    m_title = new QLabel(card);
    m_title->setStyleSheet(QString(
                               "color:%1;font-size:28px;font-weight:bold;border:none;background:transparent;"
                               "font-family:%2;").arg(kTextPrimary, kDisplayFont));
    m_title->setWordWrap(true);
    m_author = new QLabel(card);
    m_author->setStyleSheet(QString("color:%1;font-size:15px;border:none;background:transparent;").arg(kTextDim));
    m_publisher = new QLabel(card);
    m_publisher->setStyleSheet(QString("color:%1;font-size:12px;border:none;background:transparent;").arg(kTextFaint));
    m_rating = new QLabel(card);
    m_rating->setTextFormat(Qt::RichText);
    m_rating->setStyleSheet("border:none;background:transparent;");
    m_price = new QLabel(card);
    m_price->setStyleSheet(QString("color:%1;font-size:24px;font-weight:bold;border:none;background:transparent;").arg(kGold));

    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);

    m_cartBtn = new QPushButton("🛒  Add to Cart", card);
    m_cartBtn->setStyleSheet(primaryButtonQss(kAccent, "#6E3357", kAccentHover, "#82406A"));

    m_openBtn = new QPushButton("📖  Open Book", card);
    m_openBtn->setStyleSheet(primaryButtonQss(kGreen, "#2E8A5A", kGreenHover, kGreen));
    m_openBtn->hide();

    m_wishBtn = new QPushButton("🤍  Wishlist", card);
    setWishlisted(false);   // applies default style

    for (auto *b : {m_cartBtn, m_openBtn, m_wishBtn})
        b->setCursor(Qt::PointingHandCursor);

    connect(m_cartBtn, &QPushButton::clicked, this, [this] { emit addToCartRequested(m_book.id); });
    connect(m_openBtn, &QPushButton::clicked, this, [this] { emit openBookRequested(m_book.id); });
    connect(m_wishBtn, &QPushButton::clicked, this, [this] { emit wishlistToggleRequested(m_book.id); });

    btnRow->addWidget(m_cartBtn);
    btnRow->addWidget(m_openBtn);
    btnRow->addWidget(m_wishBtn);
    btnRow->addStretch();

    info->addLayout(genreWrap);
    info->addWidget(m_title);
    info->addWidget(m_author);
    info->addWidget(m_publisher);
    info->addWidget(m_rating);
    info->addSpacing(6);
    info->addWidget(m_price);
    info->addSpacing(12);
    info->addLayout(btnRow);
    info->addStretch();

    h->addWidget(m_cover);
    h->addLayout(info, 1);
    return card;
}

QWidget *BookDetailsPage::buildDescriptionCard()
{
    auto *card = new QWidget(this);
    styleCard(card, /*hero=*/false);
    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(24, 18, 24, 22);
    auto *head = new QLabel("📄  About this book", card);
    head->setStyleSheet(sectionHeadQss());
    m_desc = new QLabel(card);
    m_desc->setStyleSheet(QString("color:%1;font-size:13px;border:none;background:transparent;").arg(kTextDim));
    m_desc->setWordWrap(true);
    v->addWidget(head);
    v->addSpacing(14);
    v->addWidget(m_desc);
    return card;
}

QWidget *BookDetailsPage::buildWriteReviewCard()
{
    auto *card = new QWidget(this);
    styleCard(card, /*hero=*/false);
    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(24, 18, 24, 22);
    v->setSpacing(10);
    auto *head = new QLabel("✍️  Write a review", card);
    head->setStyleSheet(sectionHeadQss());
    v->addWidget(head);
    v->addSpacing(4);

    auto *starRow = new QHBoxLayout;
    starRow->setSpacing(6);
    m_starBtns.clear();
    for (int i = 1; i <= 5; ++i) {
        auto *star = new QPushButton("☆", card);
        star->setFixedSize(40, 40);
        star->setCursor(Qt::PointingHandCursor);
        star->setStyleSheet(QString(
                                "QPushButton{background:transparent;border:none;border-radius:20px;font-size:24px;color:%1;}"
                                "QPushButton:hover{color:%2;background:rgba(232,184,75,0.12);}").arg(kTextFaint, kGold));
        connect(star, &QPushButton::clicked, this, [this, i] { setMyRating(i); });
        m_starBtns.push_back(star);
        starRow->addWidget(star);
    }
    starRow->addStretch();
    v->addLayout(starRow);

    m_reviewEdit = new QTextEdit(card);
    m_reviewEdit->setPlaceholderText("Share your thoughts about this book...");
    m_reviewEdit->setFixedHeight(84);
    m_reviewEdit->setStyleSheet(QString(
                                    "QTextEdit{background-color:%1;border:1px solid %2;border-radius:%3px;"
                                    "padding:10px;color:%4;font-size:13px;}"
                                    "QTextEdit:focus{border:1px solid %5;}")
                                    .arg(kSurfaceAlt, kBorder).arg(kRadiusBtn).arg(kTextPrimary, kAccent));
    v->addWidget(m_reviewEdit);

    auto *submitRow = new QHBoxLayout;
    auto *submitBtn = new QPushButton("Submit Review", card);
    submitBtn->setCursor(Qt::PointingHandCursor);
    submitBtn->setStyleSheet(primaryButtonQss(kAccent, "#6E3357", kAccentHover, "#82406A"));
    connect(submitBtn, &QPushButton::clicked, this, [this] {
        emit reviewSubmitted(m_book.id, m_myRating, m_reviewEdit->toPlainText().trimmed());
    });
    submitRow->addStretch();
    submitRow->addWidget(submitBtn);
    v->addLayout(submitRow);
    return card;
}

QWidget *BookDetailsPage::buildReviewsCard()
{
    auto *card = new QWidget(this);
    styleCard(card, /*hero=*/false);
    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(24, 18, 24, 22);
    auto *head = new QLabel("💬  Reviews", card);
    head->setStyleSheet(sectionHeadQss());
    v->addWidget(head);
    v->addSpacing(14);
    m_reviewsLayout = new QVBoxLayout;
    m_reviewsLayout->setSpacing(10);
    v->addLayout(m_reviewsLayout);
    showReviews({});   // placeholder
    return card;
}

void BookDetailsPage::setBook(const Book &b)
{
    m_book = b;
    m_cover->setPixmap(coverPixmap(b, QSize(200, 290)));
    m_genre->setText(b.genre.isEmpty() ? "Book" : b.genre);
    m_title->setText(b.title);
    m_author->setText("by " + b.author);
    m_publisher->setText(b.publisherName.isEmpty() ? "" : "Published by " + b.publisherName);
    m_rating->setText(QString(
                          "<span style=\"color:%1;letter-spacing:2px;font-size:15px;\">%2</span>"
                          "&nbsp;&nbsp;<span style=\"color:%3;font-size:12px;\">%4 / 5</span>")
                          .arg(kGold, starGlyphs(b.averageRating), kTextDim, QString::number(b.averageRating, 'f', 1)));
    m_price->setText(b.price <= 0 ? "Free to read" : "$" + QString::number(b.price, 'f', 2));
    m_desc->setText(b.description.isEmpty() ? "No description available." : b.description);
    clearReviewForm();
    setOwned(false);
    showReviews({});
}

void BookDetailsPage::setOwned(bool owned)
{
    m_cartBtn->setVisible(!owned);
    m_openBtn->setVisible(owned);
}

void BookDetailsPage::setWishlisted(bool on)
{
    m_wishlisted = on;
    m_wishBtn->setText(on ? "💖  Wishlisted" : "🤍  Wishlist");
    m_wishBtn->setStyleSheet(on
                                 ? QString("QPushButton{background:transparent;border:1px solid %1;border-radius:%2px;"
                                           "padding:12px 20px;color:%1;font-size:14px;}"
                                           "QPushButton:hover{border-color:%3;color:%3;}").arg(kPink).arg(kRadiusBtn).arg("#F0A0C0")
                                 : QString("QPushButton{background:transparent;border:1px solid %1;border-radius:%2px;"
                                           "padding:12px 20px;color:%3;font-size:14px;}"
                                           "QPushButton:hover{border-color:%4;color:%4;}").arg(kBorder).arg(kRadiusBtn).arg(kTextDim, kPink));
}

void BookDetailsPage::showReviews(const QVector<Review> &reviews)
{
    while (QLayoutItem *it = m_reviewsLayout->takeAt(0)) {
        if (it->widget()) it->widget()->deleteLater();
        delete it;
    }
    if (reviews.isEmpty()) {
        auto *empty = new QLabel("🖊️  No reviews yet — be the first to share your thoughts.", this);
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet(QString(
                                 "color:%1;font-size:12px;font-style:italic;border:1px dashed %2;"
                                 "border-radius:%3px;padding:20px;background:transparent;")
                                 .arg(kTextFaint, kBorder).arg(kRadiusBtn));
        m_reviewsLayout->addWidget(empty);
        return;
    }
    for (const Review &r : reviews) {
        auto *row = new QWidget(this);
        row->setStyleSheet(QString(
                               "background-color:%1;border:1px solid %2;border-left:3px solid %3;border-radius:%4px;")
                               .arg(kSurfaceAlt, kBorder, kGold).arg(kRadiusBtn));
        auto *rv = new QVBoxLayout(row);
        rv->setContentsMargins(16, 12, 16, 12);
        rv->setSpacing(5);
        auto *stars = new QLabel(QString("<span style=\"color:%1;\">%2</span>")
                                     .arg(kGold, QString("★").repeated(r.rating) + QString("☆").repeated(5 - r.rating)), row);
        stars->setTextFormat(Qt::RichText);
        stars->setStyleSheet("font-size:13px;border:none;background:transparent;");
        auto *comment = new QLabel(r.comment, row);
        comment->setStyleSheet(QString("color:%1;font-size:12px;border:none;background:transparent;").arg(kTextPrimary));
        comment->setWordWrap(true);
        auto *date = new QLabel(r.date.toString("MMM d, yyyy"), row);
        date->setStyleSheet(QString("color:%1;font-size:10px;border:none;background:transparent;").arg(kTextFaint));
        rv->addWidget(stars);
        rv->addWidget(comment);
        rv->addWidget(date);
        m_reviewsLayout->addWidget(row);
    }
}

void BookDetailsPage::clearReviewForm()
{
    m_myRating = 0;
    m_reviewEdit->clear();
    setMyRating(0);
}

void BookDetailsPage::setMyRating(int rating)
{
    m_myRating = rating;
    for (int j = 0; j < m_starBtns.size(); ++j) {
        m_starBtns[j]->setText(j < rating ? "★" : "☆");
        m_starBtns[j]->setStyleSheet(QString(
                                         "QPushButton{background:transparent;border:none;border-radius:20px;font-size:24px;color:%1;}"
                                         "QPushButton:hover{color:%2;background:rgba(232,184,75,0.12);}")
                                         .arg(j < rating ? kGold : kTextFaint, kGold));
    }
}
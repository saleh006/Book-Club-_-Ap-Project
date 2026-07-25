#include "shelfcard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QCursor>
#include <QHash>

QString shelfIconForTitle(const QString &title)
{
    const QString t = title.trimmed().toLower();
    if (t == "favorites" || t == "favorite")   return QStringLiteral("\u2B50");
    if (t == "reading")                        return QStringLiteral("\U0001F4D6");
    if (t == "finished")                       return QStringLiteral("\u2705");
    if (t == "psychology")                     return QStringLiteral("\U0001F9E0");
    if (t == "horror")                         return QStringLiteral("\U0001F480");
    if (t == "fantasy")                        return QStringLiteral("\U0001F319");
    if (t == "business")                       return QStringLiteral("\U0001F4BC");
    if (t == "technology")                     return QStringLiteral("\U0001F4BB");
    if (t == "romance")                        return QStringLiteral("\U0001F495");
    if (t == "mystery")                        return QStringLiteral("\U0001F50D");
    return QStringLiteral("\U0001F4C1");
}

QColor shelfColorForTitle(const QString &title)
{
    const QString t = title.trimmed().toLower();
    if (t == "favorites" || t == "favorite")   return QColor("#F0B429");
    if (t == "reading")                        return QColor("#6C8EF5");
    if (t == "finished")                       return QColor("#45C48A");
    if (t == "psychology")                     return QColor("#F2789F");
    if (t == "horror")                         return QColor("#9CA3AF");
    if (t == "fantasy")                        return QColor("#8B7CF6");
    if (t == "business")                       return QColor("#D08A4C");
    if (t == "technology")                     return QColor("#4FC3D9");

    static const QVector<QColor> palette = {
        QColor("#F0B429"), QColor("#6C8EF5"), QColor("#45C48A"), QColor("#F2789F"),
        QColor("#9CA3AF"), QColor("#8B7CF6"), QColor("#D08A4C"), QColor("#4FC3D9")
    };
    const uint h = qHash(title);
    return palette[h % static_cast<uint>(palette.size())];
}

// ---------------------------------------------------------------- ShelfCard

ShelfCard::ShelfCard(const ShelfSummary &summary, const CoverProvider &coverProvider, QWidget *parent)
    : HoverCard(parent), m_summary(summary)
{
    setBackgroundColor(QColor("#181320"));
    setBorderColors(QColor(255, 255, 255, 20), QColor("#A855F7"));
    setCornerRadius(16);
    setFixedSize(176, 176);

    const QColor accent = shelfColorForTitle(m_summary.shelf.title);

    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(14, 12, 14, 14);
    v->setSpacing(8);

    auto *topRow = new QHBoxLayout;
    auto *iconLbl = new QLabel(shelfIconForTitle(m_summary.shelf.title), this);
    iconLbl->setFixedSize(32, 32);
    iconLbl->setAlignment(Qt::AlignCenter);
    iconLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
    iconLbl->setStyleSheet(QString(
                               "background-color: rgba(%1,%2,%3,45); border-radius:9px; font-size:15px; border:none;")
                               .arg(accent.red()).arg(accent.green()).arg(accent.blue()));

    auto *menuBtn = new QPushButton(QStringLiteral("\u22EE"), this);
    menuBtn->setFixedSize(24, 24);
    menuBtn->setCursor(Qt::PointingHandCursor);
    menuBtn->setStyleSheet(
        "QPushButton{background:transparent;border:none;color:rgba(255,255,255,140);font-size:14px;}"
        "QPushButton:hover{color:#FFFFFF;background-color:rgba(255,255,255,20);border-radius:6px;}");
    connect(menuBtn, &QPushButton::clicked, this, &ShelfCard::showMenu);

    topRow->addWidget(iconLbl);
    topRow->addStretch();
    topRow->addWidget(menuBtn);
    v->addLayout(topRow);

    auto *nameLbl = new QLabel(m_summary.shelf.title, this);
    nameLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
    nameLbl->setStyleSheet("color:#FFFFFF; font-size:14px; font-weight:700; background:transparent; border:none;");
    v->addWidget(nameLbl);

    auto *countLbl = new QLabel(QString("%1 Books").arg(m_summary.bookCount), this);
    countLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
    countLbl->setStyleSheet("color: rgba(255,255,255,140); font-size:11px; background:transparent; border:none;");
    v->addWidget(countLbl);

    v->addStretch();

    auto *previewRow = new QHBoxLayout;
    previewRow->setContentsMargins(0, 0, 0, 0);
    previewRow->setSpacing(4);
    static const QStringList placeholderColors = {"#3B2A4D", "#4D2A3E", "#2A3E4D", "#2A4D3B"};
    for (int i = 0; i < 4; ++i) {
        auto *thumb = new QLabel(this);
        thumb->setFixedSize(30, 42);
        thumb->setAttribute(Qt::WA_TransparentForMouseEvents);
        thumb->setScaledContents(true);

        QPixmap pix;
        if (i < m_summary.previewBooks.size() && coverProvider)
            pix = coverProvider(m_summary.previewBooks[i], QSize(30, 42));

        if (!pix.isNull()) {
            thumb->setStyleSheet("border-radius:5px;");
            thumb->setPixmap(pix);
        } else {
            thumb->setStyleSheet(QString("border-radius:5px; background-color:%1;")
                                     .arg(i < m_summary.previewBooks.size() ? placeholderColors[i % placeholderColors.size()]
                                                                            : QStringLiteral("#2A2233")));
        }
        previewRow->addWidget(thumb);
    }
    v->addLayout(previewRow);

    connect(this, &HoverCard::clicked, this, [this] { emit openRequested(m_summary.shelf.id); });
}

void ShelfCard::showMenu()
{
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu{background-color:#181320;border:1px solid rgba(255,255,255,30);"
        "border-radius:10px;padding:6px;color:#EAEAEA;}"
        "QMenu::item{padding:8px 14px;border-radius:6px;}"
        "QMenu::item:selected{background-color:#A855F7;color:white;}");
    QAction *editAction = menu.addAction("Edit Shelf");
    QAction *deleteAction = menu.addAction("Delete Shelf");

    QAction *chosen = menu.exec(QCursor::pos());
    if (chosen == editAction)
        emit editRequested(m_summary.shelf.id);
    else if (chosen == deleteAction)
        emit deleteRequested(m_summary.shelf.id);
}

// ------------------------------------------------------------- AddShelfCard

AddShelfCard::AddShelfCard(QWidget *parent) : HoverCard(parent)
{
    setBackgroundColor(QColor(255, 255, 255, 6));
    setBorderColors(QColor(0, 0, 0, 0), QColor(0, 0, 0, 0));  // painted manually below
    setCornerRadius(16);
    setFixedSize(176, 176);

    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(14, 14, 14, 14);
    v->setAlignment(Qt::AlignCenter);

    auto *plus = new QLabel(QStringLiteral("+"), this);
    plus->setAlignment(Qt::AlignCenter);
    plus->setAttribute(Qt::WA_TransparentForMouseEvents);
    plus->setStyleSheet("color:#A855F7; font-size:30px; font-weight:300; background:transparent; border:none;");

    auto *lbl = new QLabel(QStringLiteral("Create New Shelf"), this);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setAttribute(Qt::WA_TransparentForMouseEvents);
    lbl->setStyleSheet("color:#A855F7; font-size:12px; font-weight:600; background:transparent; border:none;");

    v->addWidget(plus);
    v->addWidget(lbl);

    connect(this, &HoverCard::clicked, this, &AddShelfCard::createRequested);
}

void AddShelfCard::paintEvent(QPaintEvent *event)
{
    HoverCard::paintEvent(event);   // base rounded fill (border is transparent)

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPen dashPen(QColor("#A855F7"), 1.4, Qt::DashLine);
    p.setPen(dashPen);

    const QRectF r = rect().adjusted(1, 1, -1, -1);
    QPainterPath path;
    path.addRoundedRect(r, 16, 16);
    p.drawPath(path);
}
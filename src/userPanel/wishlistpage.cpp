#include "wishlistpage.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

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
    auto *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(14, 14, 14, 14);
    rootLayout->setSpacing(16);

    auto *coverContainer = new QWidget(this);
    coverContainer->setFixedSize(72, 96);
    coverContainer->setStyleSheet("background: transparent; border: none;");
    auto *cover = makeCoverLabel(coverContainer, QSize(72, 96));
    cover->setGeometry(0, 0, 72, 96);
    rootLayout->addWidget(coverContainer);

    auto *infoLayout = new QVBoxLayout;
    infoLayout->setContentsMargins(0, 2, 0, 2);
    infoLayout->setSpacing(3);

    auto *titleLabel = new QLabel(m_item.title, this);
    titleLabel->setStyleSheet("color: #f4f1f6; font-size: 16px; font-weight: 700;");
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
    rightLayout->setSpacing(8);

    auto *priceLabel = new QLabel(m_item.price <= 0.0 ? tr("Free") : QStringLiteral("$%1").arg(m_item.price, 0, 'f', 2), this);
    priceLabel->setAlignment(Qt::AlignRight);
    priceLabel->setStyleSheet("color: #e0559e; font-size: 16px; font-weight: 700;");
    rightLayout->addWidget(priceLabel);
    rightLayout->addStretch();

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

    rootLayout->addLayout(rightLayout);
}
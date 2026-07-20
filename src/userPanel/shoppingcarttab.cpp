#include "shoppingcarttab.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QJsonDocument>
#include <QTcpSocket>


CartItemWidget::CartItemWidget(const CartDisplayItem &item, QWidget *parent)
    : QFrame(parent), m_item(item)
{
    buildUi();
}
void CartItemWidget::buildUi() {
    setObjectName("cartItemCard");
    setStyleSheet(
        "QFrame#cartItemCard { background-color: #1d1a21; border: 1px solid #2c2731; border-radius: 12px; }"
        "QFrame#cartItemCard:hover { border-color: #3a3341; }"
        "QLabel { background: transparent; border: none; }");

    auto *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(18);

    auto *coverContainer  = new QWidget(this);
    coverContainer->setFixedSize(84, 112);

    auto *cover = new QLabel(coverContainer);
    cover->setGeometry(0,0,84, 112);
    cover->setAlignment(Qt::AlignCenter);
    cover->setWordWrap(true);
    cover->setStyleSheet(
        "background-color: #2e2735; border-radius: 8px; color: #9d94a6; font-size: 11px; padding: 4px;");
    if (!m_item.cover.isNull())
        cover->setPixmap(m_item.cover.scaled(cover->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    else
        cover->setText(m_item.title);

    m_favoriteButton = new QPushButton(coverContainer);
    m_favoriteButton->setCursor(Qt::PointingHandCursor);
    m_favoriteButton->setFixedSize(24, 24);
    m_favoriteButton->move(coverContainer->width() - m_favoriteButton->width() - 4, 4);
    m_favoriteButton->raise();
    connect(m_favoriteButton, &QPushButton::clicked, this, [this] {
        m_item.favorite = !m_item.favorite;
        updateFavoriteButton();
        emit favoriteToggled(m_item.bookId, m_item.favorite);
    });
    updateFavoriteButton();
    rootLayout->addWidget(coverContainer, 0, Qt::AlignTop);

    auto *infoLayout = new QVBoxLayout;
    infoLayout->setContentsMargins(0, 2, 0, 2);
    infoLayout->setSpacing(3);

    auto *titleLabel = new QLabel(m_item.title, this);
    titleLabel->setStyleSheet("color: #f4f1f6; font-size: 19px; font-weight: 700;");
    infoLayout->addWidget(titleLabel);

    auto *authorLabel = new QLabel(m_item.author, this);
    authorLabel->setStyleSheet("color: #9d94a6; font-size: 13px;");
    infoLayout->addWidget(authorLabel);

    auto *formatLabel = new QLabel(tr("Format: %1").arg(m_item.format), this);
    formatLabel->setStyleSheet("color: #9d94a6; font-size: 13px;");
    infoLayout->addWidget(formatLabel);

    if (m_item.quantity > 1) {
        auto *qtyLabel = new QLabel(tr("Quantity: %1").arg(m_item.quantity), this);
        qtyLabel->setStyleSheet("color: #9d94a6; font-size: 13px;");
        infoLayout->addWidget(qtyLabel);
    }

    auto *priceRow = new QHBoxLayout;
    priceRow->setContentsMargins(0, 6, 0, 0);
    priceRow->setSpacing(8);
    if (m_item.originalPrice > m_item.price) {
        auto *oldPrice = new QLabel(QStringLiteral("$%1").arg(m_item.originalPrice, 0, 'f', 2), this);
        oldPrice->setStyleSheet("color: #7f7689; font-size: 14px; text-decoration: line-through;");
        priceRow->addWidget(oldPrice);
    }
    auto *price = new QLabel(QStringLiteral("$%1").arg(m_item.price, 0, 'f', 2), this);
    price->setStyleSheet("color: #d97fc4; font-size: 18px; font-weight: 700;");
    priceRow->addWidget(price);
    priceRow->addStretch();
    infoLayout->addLayout(priceRow);

    auto *actionRow = new QHBoxLayout;
    actionRow->setContentsMargins(0, 8, 0, 0);
    actionRow->setSpacing(10);

    auto *removeBtn = new QPushButton(tr("🗑  Remove"), this);
    removeBtn->setCursor(Qt::PointingHandCursor);
    removeBtn->setStyleSheet(
        "QPushButton { color: #e46060; background: transparent; border: 1px solid rgba(228, 96, 96, 0.55);"
        " border-radius: 6px; padding: 6px 14px; font-size: 13px; }"
        "QPushButton:hover { background-color: rgba(228, 96, 96, 0.12); }");
    connect(removeBtn, &QPushButton::clicked, this, [this] { emit removeRequested(m_item.bookId); });

    auto *saveBtn = new QPushButton(tr("♡  Save for Later"), this);
    saveBtn->setCursor(Qt::PointingHandCursor);
    saveBtn->setStyleSheet(
        "QPushButton { color: #c9c2d1; background: transparent; border: 1px solid #453f4d;"
        " border-radius: 6px; padding: 6px 14px; font-size: 13px; }"
        "QPushButton:hover { background-color: rgba(201, 194, 209, 0.08); }");
    connect(saveBtn, &QPushButton::clicked, this, [this] { emit saveForLaterRequested(m_item.bookId); });

    actionRow->addWidget(removeBtn);
    actionRow->addWidget(saveBtn);
    actionRow->addStretch();
    infoLayout->addLayout(actionRow);

    rootLayout->addLayout(infoLayout, 1);
}
void CartItemWidget::updateFavoriteButton() {
    m_favoriteButton->setText(m_item.favorite ? QStringLiteral("♥") : QStringLiteral("♡"));
    m_favoriteButton->setStyleSheet(QString(
                                        "QPushButton { border: none; border-radius: 12px; font-size: 15px; color: %1;"
                                        " background-color: rgba(176, 73, 143, 40); }"
                                        "QPushButton:hover { background-color: rgba(176, 73, 143, 90); }")
                                        .arg(m_item.favorite ? "#d9a8e0" : "#9d94a6"));
}

OrderSummaryWidget::OrderSummaryWidget(QWidget *parent)
    : QFrame(parent)
{
    buildUi();
}
QLabel *OrderSummaryWidget::makeRowLabel(const QString &text, const QString &color, QWidget *parent) {
    auto *label = new QLabel(text, parent);
    label->setStyleSheet(QString("color: %1; font-size: 14px;").arg(color));
    return label;
}
void OrderSummaryWidget::buildUi() {
    setObjectName("orderSummaryCard");
    setFixedWidth(300);
    setStyleSheet(
        "QFrame#orderSummaryCard { background-color: #1d1a21; border: 1px solid #2c2731; border-radius: 12px; }"
        "QLabel { background: transparent; border: none; }");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(22, 22, 22, 22);
    layout->setSpacing(14);

    auto *titleLabel = new QLabel(tr("📒  Order Summary"), this);
    titleLabel->setStyleSheet("color: #f4f1f6; font-size: 19px; font-weight: 700;");
    layout->addWidget(titleLabel);

    auto *rows = new QGridLayout;
    rows->setContentsMargins(0, 4, 0, 4);
    rows->setHorizontalSpacing(12);
    rows->setVerticalSpacing(12);
    rows->setColumnStretch(0, 1);

    m_itemsCaption  = makeRowLabel(tr("Items (0):"), "#9d94a6", this);
    m_itemsValue    = makeRowLabel("$0.00", "#f4f1f6", this);
    m_discountValue = makeRowLabel("-$0.00", "#e46060", this);
    m_subtotalValue = makeRowLabel("$0.00", "#f4f1f6", this);

    rows->addWidget(m_itemsCaption, 0, 0);
    rows->addWidget(m_itemsValue, 0, 1, Qt::AlignRight);
    rows->addWidget(makeRowLabel(tr("Discount:"), "#9d94a6", this), 1, 0);
    rows->addWidget(m_discountValue, 1, 1, Qt::AlignRight);
    rows->addWidget(makeRowLabel(tr("Subtotal:"), "#9d94a6", this), 2, 0);
    rows->addWidget(m_subtotalValue, 2, 1, Qt::AlignRight);
    layout->addLayout(rows);

    auto *divider = new QFrame(this);
    divider->setFrameShape(QFrame::HLine);
    divider->setFixedHeight(1);
    divider->setStyleSheet("background-color: #2c2731; border: none;");
    layout->addWidget(divider);

    auto *totalRow = new QHBoxLayout;
    auto *totalCaption = new QLabel(tr("Total Payable:"), this);
    totalCaption->setStyleSheet("color: #f4f1f6; font-size: 16px; font-weight: 700;");
    m_totalValue = new QLabel("$0.00", this);
    m_totalValue->setStyleSheet("color: #f4f1f6; font-size: 22px; font-weight: 800;");
    totalRow->addWidget(totalCaption);
    totalRow->addStretch();
    totalRow->addWidget(m_totalValue);
    layout->addLayout(totalRow);

    m_checkoutButton = new QPushButton(tr("💳  Proceed to Checkout"), this);
    m_checkoutButton->setCursor(Qt::PointingHandCursor);
    m_checkoutButton->setMinimumHeight(46);
    m_checkoutButton->setStyleSheet(
        "QPushButton { color: #ffffff; background-color: #b0498f; border: none; border-radius: 8px;"
        " font-size: 15px; font-weight: 700; }"
        "QPushButton:hover { background-color: #c25aa2; }"
        "QPushButton:pressed { background-color: #93407a; }"
        "QPushButton:disabled { background-color: #2c2731; color: #7f7689; }");

    // connect(m_checkoutButton, &QPushButton::clicked, this, &OrderSummaryWidget::checkoutRequested);

    layout->addSpacing(4);
    layout->addWidget(m_checkoutButton);
}
void OrderSummaryWidget::updateSummary(int itemCount, double itemsTotal, double discount) {
    const double subtotal = itemsTotal - discount;

    m_itemsCaption->setText(tr("Items (%1):").arg(itemCount));
    m_itemsValue->setText(QStringLiteral("$%1").arg(itemsTotal, 0, 'f', 2));
    m_discountValue->setText(QStringLiteral("-$%1").arg(discount, 0, 'f', 2));
    m_subtotalValue->setText(QStringLiteral("$%1").arg(subtotal, 0, 'f', 2));
    m_totalValue->setText(QStringLiteral("$%1").arg(subtotal, 0, 'f', 2));
    m_checkoutButton->setEnabled(itemCount > 0);
}


ShoppingCartPage::ShoppingCartPage(QTcpSocket *socket, int userId, QWidget *parent)
    : QWidget(parent), m_socket(socket), m_userId(userId)
{
    buildUi();
}

void ShoppingCartPage::buildUi()
{
    setObjectName("shoppingCartPage");
    setStyleSheet("QWidget#shoppingCartPage { background-color: #141216; }");

    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(28, 24, 28, 24);
    pageLayout->setSpacing(18);

    m_headerLabel = new QLabel(this);
    m_headerLabel->setStyleSheet("color: #f4f1f6; font-size: 26px; font-weight: 800; background: transparent;");
    m_headerLabel->setText(tr("🛒  Shopping Cart"));
    pageLayout->addWidget(m_headerLabel);

    auto *bodyLayout = new QHBoxLayout;
    bodyLayout->setSpacing(22);

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

    auto *listContainer = new QWidget(m_scrollArea);
    listContainer->setStyleSheet("background: transparent;");
    m_itemsLayout = new QVBoxLayout(listContainer);
    m_itemsLayout->setContentsMargins(0, 0, 8, 0); // right gap for scrollbar
    m_itemsLayout->setSpacing(14);

    m_emptyLabel = new QLabel(tr("Your cart is empty."), listContainer);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("color: #7f7689; font-size: 15px; padding: 40px; background: transparent;");
    m_itemsLayout->addWidget(m_emptyLabel);
    m_itemsLayout->addStretch();

    m_scrollArea->setWidget(listContainer);
    bodyLayout->addWidget(m_scrollArea, 1);

    m_summaryWidget = new OrderSummaryWidget(this);

    auto *summaryColumn = new QVBoxLayout;
    summaryColumn->addWidget(m_summaryWidget);
    summaryColumn->addStretch();
    bodyLayout->addLayout(summaryColumn);

    pageLayout->addLayout(bodyLayout, 1);
}

void ShoppingCartPage::sendRequest(const QString &action, const QJsonObject &extra)
{
    if (!m_socket)
        return;

    QJsonObject packet = extra;
    packet["action"] = action;
    packet["userId"] = m_userId;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
    m_socket->flush();
}

void ShoppingCartPage::refreshCart() {
    sendRequest("cart_fetch");
}

void ShoppingCartPage::handleServerResponse(const QJsonObject &response)
{
    const QString action = response["action"].toString();

    if (action == "cart_fetch_response") {
        handleCartFetchResponse(response);
    } else if (action == "checkout_response") {
        handleCheckoutResponse(response);
    } else if (action == "add_to_cart_response"
               || action == "remove_from_cart_response"
               || action == "cart_clear_response"
               || action == "wishlist_add_response"
               || action == "wishlist_remove_response") {
        handleMutationResponse(response);
    }
}

void ShoppingCartPage::handleCartFetchResponse(const QJsonObject &response)
{
    if (response["status"].toString() != "success") {
        QMessageBox::warning(this, tr("Cart"), response["message"].toString());
        return;
    }

    QList<CartDisplayItem> items;
    const QJsonArray rawItems = response["items"].toArray();
    for (const QJsonValue &value : rawItems) {
        const QJsonObject o = value.toObject();
        CartDisplayItem item;
        item.bookId = o["bookId"].toInt();
        item.title = o["title"].toString();
        item.author = o["author"].toString();
        item.quantity = qMax(1, o["quantity"].toInt(1));
        item.originalPrice = o["originalPrice"].toDouble();
        item.price = o["price"].toDouble();
        item.favorite = o["favorite"].toBool();

        const QString coverPath = o["coverImagePath"].toString();
        if (!coverPath.isEmpty()) {
            QPixmap pm(coverPath);
            if (!pm.isNull())
                item.cover = pm;
        }
        items.append(item);
    }

    setItems(items);
}

void ShoppingCartPage::handleMutationResponse(const QJsonObject &response)
{
    if (response["status"].toString() != "success") {
        QMessageBox::warning(this, tr("Cart"), response["message"].toString());
        return;
    }
    refreshCart();
}

void ShoppingCartPage::handleCheckoutResponse(const QJsonObject &response)
{
    if (response["status"].toString() != "success") {
        QMessageBox::warning(this, tr("Checkout"), response["message"].toString());
        return;
    }

    const int purchaseId = response["purchaseId"].toInt();
    QMessageBox::information(this, tr("Checkout"), tr("Purchase completed successfully!"));
    emit checkoutCompleted(purchaseId);
    refreshCart();
}

void ShoppingCartPage::setItems(const QList<CartDisplayItem> &items) {
    m_items = items;
    rebuildItemList();
    refreshSummaryAndHeader();
    emit cartUpdated();
}
void ShoppingCartPage::rebuildItemList() {
    const auto oldCards = m_scrollArea->widget()->findChildren<CartItemWidget *>(
        QString(), Qt::FindDirectChildrenOnly);
    for (CartItemWidget *card : oldCards)
        card->deleteLater();

    int insertIndex = m_itemsLayout->indexOf(m_emptyLabel);
    for (const CartDisplayItem &item : m_items) {
        auto *card = new CartItemWidget(item, m_scrollArea->widget());
        connect(card, &CartItemWidget::removeRequested, this,
                [this](int bookId) { sendRequest("remove_from_cart", {{"bookId", bookId}}); });
        connect(card, &CartItemWidget::saveForLaterRequested, this,
                [this](int bookId) {
                    // Move the book to the wishlist and take it out of the cart.
                    sendRequest("wishlist_add", {{"bookId", bookId}});
                    sendRequest("remove_from_cart", {{"bookId", bookId}});
                });
        connect(card, &CartItemWidget::favoriteToggled, this,
                [this](int bookId, bool fav) {
                    sendRequest(fav ? "wishlist_add" : "wishlist_remove", {{"bookId", bookId}});
                });
        m_itemsLayout->insertWidget(insertIndex++, card);
    }

    m_emptyLabel->setVisible(m_items.isEmpty());
}
void ShoppingCartPage::refreshSummaryAndHeader() {
    double itemsTotal = 0.0;
    double discount = 0.0;
    for (const CartDisplayItem &item : m_items) {
        itemsTotal += item.originalPrice * item.quantity;
        discount += item.discount() * item.quantity;
    }

    m_headerLabel->setText(tr("🛒  Shopping Cart (%1 %2)")
                               .arg(m_items.size())
                               .arg(m_items.size() == 1 ? tr("Item") : tr("Items")));
    m_summaryWidget->updateSummary(m_items.size(), itemsTotal, discount);
}
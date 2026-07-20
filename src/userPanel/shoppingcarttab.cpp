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
    : QFrame(parent), m_item(item) {}
void CartItemWidget::buildUi() {}
void CartItemWidget::updateFavoriteButton() {}

OrderSummaryWidget::OrderSummaryWidget(QWidget *parent) : QFrame(parent) {}
QLabel *OrderSummaryWidget::makeRowLabel(const QString &text, const QString &color, QWidget *parent) { return nullptr; }
void OrderSummaryWidget::buildUi() {}
void OrderSummaryWidget::updateSummary(int itemCount, double itemsTotal, double discount) {}


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
/////////////////////////////////////////
void ShoppingCartPage::refreshCart() {}
void ShoppingCartPage::setItems(const QList<CartDisplayItem> &items) {}
void ShoppingCartPage::rebuildItemList() {}
void ShoppingCartPage::refreshSummaryAndHeader() {}
#ifndef SHOPPINGCARTPAGE_H
#define SHOPPINGCARTPAGE_H

#include <QWidget>
#include <QFrame>
#include <QPixmap>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>

class QTcpSocket;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QScrollArea;

struct CartDisplayItem
{
    int bookId = -1;
    QString title;
    QString author;
    QString format = QStringLiteral("PDF/EPUB");
    int quantity = 1;
    double originalPrice = 0.0;
    double price = 0.0;
    bool favorite= false;
    QPixmap cover;

    double discount() const { return originalPrice - price; }
};

class CartItemWidget : public QFrame
{
    Q_OBJECT
public:
    explicit CartItemWidget(const CartDisplayItem &item, QWidget *parent = nullptr);
    const CartDisplayItem &item() const { return m_item; }

signals:
    void removeRequested(int bookId);
    void saveForLaterRequested(int bookId);
    void favoriteToggled(int bookId, bool favorite);

private:
    void buildUi();
    void updateFavoriteButton();

    CartDisplayItem m_item;
    QPushButton *m_favoriteButton = nullptr;
};

class OrderSummaryWidget : public QFrame
{
    Q_OBJECT
public:
    explicit OrderSummaryWidget(QWidget *parent = nullptr);
    void updateSummary(int itemCount, double itemsTotal, double discount);

signals:
    void checkoutRequested();

private:
    void buildUi();
    static QLabel *makeRowLabel(const QString &text, const QString &color, QWidget *parent);

    QLabel *m_itemsCaption  = nullptr;
    QLabel *m_itemsValue    = nullptr;
    QLabel *m_discountValue = nullptr;
    QLabel *m_subtotalValue = nullptr;
    QLabel *m_totalValue    = nullptr;
    QPushButton *m_checkoutButton = nullptr;
};

class ShoppingCartPage : public QWidget
{
    Q_OBJECT
public:
    explicit ShoppingCartPage(QTcpSocket *socket,int userId, QWidget *parent = nullptr);
    void refreshCart();
    void handleServerResponse(const QJsonObject &response);
    bool containsBook(int bookId) const {
        for (const auto &item : m_items) {
            if (item.bookId == bookId) return true;
        }
        return false;
    }

signals:
    void checkoutCompleted(int purchaseId);
    void cartUpdated();

private:
    void buildUi();
    void sendRequest(const QString &action, const QJsonObject &extra = {});
    void setItems(const QList<CartDisplayItem> &items);
    void rebuildItemList();
    void refreshSummaryAndHeader();
    void handleCartFetchResponse(const QJsonObject &response);
    void handleMutationResponse(const QJsonObject &response);
    void handleCheckoutResponse(const QJsonObject &response);

    QTcpSocket *m_socket = nullptr;
    int m_userId = -1;

    QList<CartDisplayItem> m_items;

    QLabel *m_headerLabel = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    QVBoxLayout *m_itemsLayout = nullptr;
    QLabel *m_emptyLabel = nullptr;
    OrderSummaryWidget *m_summaryWidget = nullptr;
};

#endif // SHOPPINGCARTPAGE_H
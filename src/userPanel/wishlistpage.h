#ifndef WISHLISTPAGE_H
#define WISHLISTPAGE_H

#include <QWidget>
#include <QFrame>
#include <QPixmap>
#include <QString>
#include <QList>
#include <QVector>
#include "models.h"
#include <QJsonObject>
#include <QJsonArray>

class QTcpSocket;
class QLineEdit;
class QComboBox;
class QGridLayout;
class QScrollArea;

struct WishlistDisplayItem
{
    int bookId = -1;
    QString title;
    QString author;
    QString genre;
    QString format = QStringLiteral("eBook");
    double price = 0.0;
    bool isNew = false;
    QPixmap cover;
};

class QLabel;
class QPushButton;

class WishlistItemWidget : public QFrame
{
    Q_OBJECT
public:
    explicit WishlistItemWidget(const WishlistDisplayItem &item, bool listMode, QWidget *parent = nullptr);
    const WishlistDisplayItem &item() const { return m_item; }

signals:
    void removeRequested(int bookId);
    void addToCartRequested(int bookId);
    void detailsRequested(int bookId);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void buildGridUi();
    void buildListUi();
    QPushButton *makeCartButton();
    QPushButton *makeRemoveButton();
    QPushButton *makeHeartButton(QWidget *parent);
    QLabel *makeCoverLabel(QWidget *parent, const QSize &size);

    WishlistDisplayItem m_item;
    bool m_listMode = false;
};

class WishlistPage : public QWidget
{
    Q_OBJECT
public:
    explicit WishlistPage(QTcpSocket *socket, int userId, QWidget *parent = nullptr);
    void refreshWishlist();
    void handleServerResponse(const QJsonObject &response);
    void setCatalog(const QVector<Book> &books);
    bool containsBook(int bookId) const {
        for (const auto &item : m_items) {
            if (item.bookId == bookId) return true;
        }
        return false;
    }

signals:
    void addToCartRequested(int bookId);
    void bookDetailsRequested(int bookId);
    void exploreBooksRequested();
    void wishlistUpdated();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void buildUi();
    void sendRequest(const QString &action, const QJsonObject &extra = {});
    void handleWishlistFetchResponse(const QJsonObject &response);
    void handleMutationResponse(const QJsonObject &response);
    void removeItemLocally(int bookId);
    void setItems(const QList<WishlistDisplayItem> &items);
    void enrichFromCatalog(WishlistDisplayItem &item) const;
    QList<WishlistDisplayItem> filteredSortedItems() const;
    void rebuildGrid();
    void refreshCountLabel();
    void updateEmptyLabel(int visibleCount);
    void setListMode(bool listMode);
    QWidget *makeDiscoverCard();
    int gridColumnCount() const;

    QTcpSocket *m_socket = nullptr;
    int m_userId = -1;

    QList<WishlistDisplayItem> m_items;
    QVector<Book> m_catalog;
    int m_filterMode = 0;
    bool m_listMode = false;

    QLabel *m_countLabel = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QPushButton *m_filterButton = nullptr;
    QComboBox *m_sortCombo = nullptr;
    QPushButton *m_gridViewButton = nullptr;
    QPushButton *m_listViewButton = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    QGridLayout *m_gridLayout = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QWidget *m_discoverCard = nullptr;
    int m_columns = 1;
};

#endif // WISHLISTPAGE_H
#ifndef WISHLISTPAGE_H
#define WISHLISTPAGE_H

#include <QWidget>
#include <QFrame>
#include <QPixmap>
#include <QString>

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

#endif // WISHLISTPAGE_H
#ifndef BOOKCARD_H
#define BOOKCARD_H

#include "hovercard.h"
#include "models.h"   // Book
#include <QPixmap>
#include <QSize>
#include <functional>

class QLabel;
class QPushButton;

using CoverProvider = std::function<QPixmap(const Book &book, const QSize &size)>;

struct ContinueReadingItem
{
    Book book;
    int lastPage = 0;
    int progressPercent = 0;   // 0-100, computed by the caller
};

class BookCard : public HoverCard
{
    Q_OBJECT
public:
    explicit BookCard(const Book &book, QWidget *parent = nullptr);

    int bookId() const { return m_book.id; }
    void setFavorite(bool favorite);
    void setCoverPixmap(const QPixmap &pixmap);

signals:
    void openRequested(int bookId);
    void readRequested(int bookId);
    void moveToShelfRequested(int bookId);
    void favoriteToggleRequested(int bookId);
    void moreOptionsRequested(int bookId);

private:
    QPushButton *makeIconButton(const QString &text, const QString &tooltip);

    Book m_book;
    QLabel *m_coverLabel = nullptr;
    QPushButton *m_favoriteBtn = nullptr;
    bool m_favorite = false;
};

class ContinueReadingCard : public HoverCard
{
    Q_OBJECT
public:
    explicit ContinueReadingCard(const ContinueReadingItem &item, QWidget *parent = nullptr);

    int bookId() const { return m_item.book.id; }
    void setCoverPixmap(const QPixmap &pixmap);

signals:
    void continueRequested(int bookId);

private:
    ContinueReadingItem m_item;
    QLabel *m_coverLabel = nullptr;
};

#endif // BOOKCARD_H
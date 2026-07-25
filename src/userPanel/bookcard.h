#ifndef BOOKCARD_H
#define BOOKCARD_H

#include "hovercard.h"
#include "models.h"   // Book
#include <QPixmap>
#include <QSize>
#include <functional>

class QLabel;
class QPushButton;

// Supplies an already-rendered cover pixmap for a book (e.g. downloaded and
// cached from the server, the same way the rest of this app already does
// via makeCoverPixmap()/downloadFileFromServer() in userpanel.cpp) at the
// requested target size. The integrator supplies this; these widgets never
// touch the network themselves. Returning a null QPixmap tells the card to
// keep its built-in colored placeholder.
using CoverProvider = std::function<QPixmap(const Book &book, const QSize &size)>;

// A single item in the horizontally-scrolling "Continue Reading" strip.
// This widget is purely presentational: it does not know how progress is
// tracked server-side, it just displays whatever percentage it is given.
struct ContinueReadingItem
{
    Book book;
    int lastPage = 0;
    int progressPercent = 0;   // 0-100, computed by the caller
};

// Grid card used in "My Books" and inside a shelf's book grid. Clicking the
// cover/title/rating area opens the book; the four small action buttons at
// the bottom (Read, Move to Shelf, Favorite, More) fire their own signals.
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

// A single row-style card in the "Continue Reading" horizontal strip:
// small cover, title/author, a progress bar with percentage, and a
// circular "continue" play button.
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
#ifndef SHELFCARD_H
#define SHELFCARD_H

#include "hovercard.h"
#include "models.h"   // Shelf, Book
#include "bookcard.h"          // CoverProvider
#include <QVector>
#include <QColor>

class QLabel;
class QPushButton;

// A shelf plus everything the grid card needs to render it. The full book
// list only needs to be fetched when the shelf is actually opened, so this
// only carries a small preview (up to 4 covers) and a count.
struct ShelfSummary
{
    Shelf shelf;
    int bookCount = 0;
    QVector<Book> previewBooks;   // up to 4, used for the mini cover strip
};

// Deterministic icon/accent-color lookup by shelf title. The backend has
// no color/icon column for shelves, so well-known names (Favorites,
// Reading, Finished, ...) get a curated look and anything else falls back
// to a stable hash-based color so it still looks intentional and stays
// the same across sessions.
QString shelfIconForTitle(const QString &title);
QColor  shelfColorForTitle(const QString &title);

// Shelf tile shown in the "My Shelves" grid: icon, name, book count, a
// mini preview strip of up to 4 covers, and a 3-dot menu (Edit/Delete).
// Clicking anywhere else on the card opens the shelf.
class ShelfCard : public HoverCard
{
    Q_OBJECT
public:
    explicit ShelfCard(const ShelfSummary &summary, const CoverProvider &coverProvider = CoverProvider(),
                       QWidget *parent = nullptr);

    int shelfId() const { return m_summary.shelf.id; }

signals:
    void openRequested(int shelfId);
    void editRequested(int shelfId);
    void deleteRequested(int shelfId);

private:
    void showMenu();

    ShelfSummary m_summary;
};

// The dashed "+ Create New Shelf" tile at the end of the shelves grid.
class AddShelfCard : public HoverCard
{
    Q_OBJECT
public:
    explicit AddShelfCard(QWidget *parent = nullptr);

signals:
    void createRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // SHELFCARD_H
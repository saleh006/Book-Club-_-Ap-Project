#ifndef SHELFDETAILPAGE_H
#define SHELFDETAILPAGE_H

#include <QWidget>
#include <QVector>
#include "shelfcard.h"   // ShelfSummary
#include "bookcard.h"    // BookCard

class QLabel;
class QGridLayout;

// Full-page view of a single shelf: back button, shelf icon/name/count,
// Edit/Delete actions, and a responsive grid of BookCards. Swapped in by
// MyLibraryPage's internal QStackedWidget when a shelf is opened.
class ShelfDetailPage : public QWidget
{
    Q_OBJECT
public:
    explicit ShelfDetailPage(QWidget *parent = nullptr);

    void setShelf(const ShelfSummary &summary, const QVector<Book> &books);
    void setCoverProvider(const CoverProvider &provider);

signals:
    void backRequested();
    void editRequested(int shelfId);
    void deleteRequested(int shelfId);
    void bookOpenRequested(int bookId);
    void bookReadRequested(int bookId);
    void bookMoveRequested(int bookId);
    void bookFavoriteToggleRequested(int bookId);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void rebuildGrid();
    void relayoutGrid();

    ShelfSummary m_summary;
    QVector<Book> m_books;
    CoverProvider m_coverProvider;

    QLabel *m_iconLbl = nullptr;
    QLabel *m_nameLbl = nullptr;
    QLabel *m_countLbl = nullptr;

    QGridLayout *m_grid = nullptr;
    QWidget *m_gridHost = nullptr;
    QVector<BookCard *> m_cards;
};

#endif // SHELFDETAILPAGE_H
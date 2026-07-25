#ifndef SHELFDETAILPAGE_H
#define SHELFDETAILPAGE_H

#include <QWidget>
#include <QVector>
#include "shelfcard.h"   // ShelfSummary
#include "bookcard.h"    // BookCard

class QLabel;
class QGridLayout;

class ShelfDetailPage : public QWidget
{
    Q_OBJECT
public:
    explicit ShelfDetailPage(QWidget *parent = nullptr);

    void setShelf(const ShelfSummary &summary, const QVector<Book> &books);
    void setCoverProvider(const CoverProvider &provider);
    int currentShelfId() const { return m_summary.shelf.id; }

signals:
    void backRequested();
    void editRequested(int shelfId);
    void deleteRequested(int shelfId);
    void bookOpenRequested(int bookId);
    void bookReadRequested(int bookId);
    void bookMoveRequested(int bookId);
    void bookFavoriteToggleRequested(int bookId);
    void bookRemoveFromShelfRequested(int bookId);

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
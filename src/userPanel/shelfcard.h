#ifndef SHELFCARD_H
#define SHELFCARD_H

#include "hovercard.h"
#include "models.h"   // Shelf, Book
#include "bookcard.h"          // CoverProvider
#include <QVector>
#include <QColor>

class QLabel;
class QPushButton;


struct ShelfSummary
{
    Shelf shelf;
    int bookCount = 0;
    QVector<Book> previewBooks;   // up to 4, used for the mini cover strip
};


QString shelfIconForTitle(const QString &title);
QColor  shelfColorForTitle(const QString &title);

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
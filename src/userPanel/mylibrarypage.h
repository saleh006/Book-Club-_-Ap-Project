#ifndef MYLIBRARYPAGE_H
#define MYLIBRARYPAGE_H

#include <QWidget>
#include <QVector>
#include <QSet>
#include <QColor>
#include "models.h"   // Book, Shelf
#include "bookcard.h"          // ContinueReadingItem
#include "shelfcard.h"         // ShelfSummary, AddShelfCard

class QLabel;
class QLineEdit;
class QComboBox;
class QPushButton;
class QGridLayout;
class QHBoxLayout;
class QStackedWidget;
class StatisticsCard;
class ShelfDetailPage;


class MyLibraryPage : public QWidget
{
    Q_OBJECT
public:
    explicit MyLibraryPage(QWidget *parent = nullptr);

    void setStatistics(int totalBooks, int totalShelves, int currentlyReading, int favorites);
    void setContinueReading(const QVector<ContinueReadingItem> &items);
    void setMyBooks(const QVector<Book> &books);
    void setShelves(const QVector<ShelfSummary> &shelves);
    void setFavoriteBookIds(const QSet<int> &bookIds);
    void setCoverProvider(const CoverProvider &provider);
    void showShelfDetail(const ShelfSummary &summary, const QVector<Book> &books);
    void backToLibrary();
    int currentlyViewedShelfId() const;

signals:
    void bookOpenRequested(int bookId);
    void bookReadRequested(int bookId);
    void bookFavoriteToggleRequested(int bookId);
    void moveBookToShelfRequested(int bookId, int shelfId);
    void createShelfRequested(const QString &name, const QString &description, const QColor &color);
    void editShelfRequested(int shelfId, const QString &newName, const QString &newDescription, const QColor &newColor);
    void deleteShelfRequested(int shelfId);
    void shelfOpened(int shelfId);
    void removeBookFromShelfRequested(int bookId, int shelfId);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QWidget *buildHeader();
    QWidget *buildTabsAndToolbar();
    QWidget *buildContinueReadingSection();
    QWidget *buildMyBooksSection();
    QWidget *buildShelvesSection();

    void setActiveTab(int index);
    void updateSectionVisibility();
    void applyFilters();

    void rebuildContinueReadingRow();
    void rebuildBooksGrid();
    void rebuildShelvesGrid();
    void relayoutBooksGrid();
    void relayoutShelvesGrid();

    void openMoveDialog(int bookId);
    void openCreateShelfDialog();
    void openEditShelfDialog(int shelfId);
    void openShelfDetail(int shelfId);

    // data
    QVector<Book> m_allBooks;
    QVector<Book> m_filteredBooks;
    QVector<ContinueReadingItem> m_continueReading;
    QVector<ShelfSummary> m_shelves;
    QSet<int> m_favoriteIds;
    CoverProvider m_coverProvider;

    int m_activeTab = 0;   // 0 All, 1 My Books, 2 Shelves
    QString m_searchText;
    QString m_sortMode = QStringLiteral("Newest");
    QString m_filterGenre = QStringLiteral("All");
    bool m_gridView = true;

    // chrome
    QStackedWidget *m_stack = nullptr;
    ShelfDetailPage *m_shelfDetail = nullptr;

    StatisticsCard *m_statBooks = nullptr;
    StatisticsCard *m_statShelves = nullptr;
    StatisticsCard *m_statReading = nullptr;
    StatisticsCard *m_statFavorites = nullptr;

    QPushButton *m_tabAll = nullptr;
    QPushButton *m_tabMyBooks = nullptr;
    QPushButton *m_tabShelves = nullptr;

    QLineEdit *m_searchEdit = nullptr;
    QComboBox *m_sortCombo = nullptr;
    QComboBox *m_filterCombo = nullptr;
    QPushButton *m_viewGridBtn = nullptr;
    QPushButton *m_viewListBtn = nullptr;

    QWidget *m_continueSection = nullptr;
    QHBoxLayout *m_continueRow = nullptr;
    QWidget *m_continueRowHost = nullptr;

    QWidget *m_booksSection = nullptr;
    QPushButton *m_booksSeeAll = nullptr;
    QGridLayout *m_booksGrid = nullptr;
    QWidget *m_booksGridHost = nullptr;

    QWidget *m_shelvesSection = nullptr;
    QGridLayout *m_shelvesGrid = nullptr;
    QWidget *m_shelvesGridHost = nullptr;

    QVector<BookCard *> m_bookCards;
    QVector<ShelfCard *> m_shelfCards;
    AddShelfCard *m_addShelfCard = nullptr;
};

#endif // MYLIBRARYPAGE_H
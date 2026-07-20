#ifndef BOOKDETAILSPAGE_H
#define BOOKDETAILSPAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QVector>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include "models.h"

class BookDetailsPage : public QWidget
{
    Q_OBJECT
public:
    explicit BookDetailsPage(QWidget *parent = nullptr);

    void setBook(const Book &b);          // populate the page
    void setOwned(bool owned);            // toggles Add-to-Cart vs Open-Book
    int currentBookId() const { return m_book.id; }

signals:
    void backRequested();
    void addToCartRequested(int bookId);
    void wishlistToggleRequested(int bookId);
    void openBookRequested(int bookId);
    void reviewSubmitted(int bookId, int rating, const QString &comment);

public slots:
    void setWishlisted(bool on);          // updates heart button look
    void showReviews(const QVector<Review> &reviews);
    void clearReviewForm();

private:
    QWidget *buildTopCard();
    QWidget *buildDescriptionCard();
    QWidget *buildWriteReviewCard();
    QWidget *buildReviewsCard();
    void setMyRating(int rating);

    Book m_book;
    bool m_wishlisted = false;
    int m_myRating = 0;

    QLabel *m_cover = nullptr;
    QLabel *m_genre = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_author = nullptr;
    QLabel *m_publisher = nullptr;
    QLabel *m_rating = nullptr;
    QLabel *m_price = nullptr;
    QLabel *m_desc = nullptr;
    QPushButton *m_cartBtn = nullptr;
    QPushButton *m_openBtn = nullptr;
    QPushButton *m_wishBtn = nullptr;
    QTextEdit *m_reviewEdit = nullptr;
    QVector<QPushButton*> m_starBtns;
    QVBoxLayout *m_reviewsLayout = nullptr;
};

class HeroCard : public QWidget {
public:
    using QWidget::QWidget;
    void setBackgroundImage(const QPixmap &pm) { m_bg = pm; update(); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath clip;
        clip.addRoundedRect(rect(), 16, 16);
        p.setClipPath(clip);

        if (!m_bg.isNull()) {
            QPixmap scaled = m_bg.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            p.drawPixmap((width() - scaled.width()) / 2, (height() - scaled.height()) / 2, scaled);
            p.fillRect(rect(), QColor(7, 5, 12, 50));   // scrim so title/buttons stay readable
        } else {
            p.fillRect(rect(), QColor("#140F1B"));
        }
        p.setPen(QPen(QColor("#241A2E"), 1));
        p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 16, 16);
    }
private:
    QPixmap m_bg;
};

#endif // BOOKDETAILSPAGE_H
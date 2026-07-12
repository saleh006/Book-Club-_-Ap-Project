#include "bookcardwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QFrame>

BookCardWidget::BookCardWidget(const Book &book, QWidget *parent)
    : QWidget(parent), m_book(book)
{
    setObjectName("bookCard");
    setFixedSize(180, 240);
    setCursor(Qt::PointingHandCursor);

    // 1. Cover Frame
    QFrame *coverFrame = new QFrame(this);
    coverFrame->setObjectName("coverFrame");
    coverFrame->setFixedSize(156, 156);

    // 2. Cover Label
    m_coverLabel = new QLabel(coverFrame);
    m_coverLabel->setAlignment(Qt::AlignCenter);
    m_coverLabel->setObjectName("bookCover");

    QVBoxLayout *frameLayout = new QVBoxLayout(coverFrame);
    frameLayout->setContentsMargins(3, 3, 3, 3);
    frameLayout->addWidget(m_coverLabel);

    QPixmap cover(book.coverImagePath);
    if (cover.isNull()) {
        m_coverLabel->setText("No Cover");
    } else {
        m_coverLabel->setPixmap(cover.scaled(150, 150, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    }

    // 2b. Status badge - sits in the top-left corner of the cover frame
    m_statusBadge = new QLabel(coverFrame);
    m_statusBadge->setObjectName("statusBadge");
    m_statusBadge->move(6, 6);
    m_statusBadge->raise();

    switch (book.status) {
    case 1:
        m_statusBadge->setText("Active");
        m_statusBadge->setStyleSheet("background-color: rgba(46,204,113,220); color: white; border-radius: 4px; padding: 2px 6px; font-size: 9px; font-weight: bold;");
        break;
    case 0:
        m_statusBadge->setText("Pending");
        m_statusBadge->setStyleSheet("background-color: rgba(241,196,15,220); color: #1a1a1a; border-radius: 4px; padding: 2px 6px; font-size: 9px; font-weight: bold;");
        break;
    default: // -1 or anything else
        m_statusBadge->setText("Deleted");
        m_statusBadge->setStyleSheet("background-color: rgba(192,57,43,220); color: white; border-radius: 4px; padding: 2px 6px; font-size: 9px; font-weight: bold;");
        break; // we can remove it
    }
    m_statusBadge->adjustSize();

    // 3. Hover Overlay
    m_overlay = new QWidget(m_coverLabel);
    m_overlay->setObjectName("cardOverlay");
    m_overlay->hide();

    m_editBtn = new QPushButton("✎ Edit", m_overlay);
    m_editBtn->setObjectName("cardEditBtn");

    m_offerBtn = new QPushButton("🏷 Set Offer", m_overlay);
    m_offerBtn->setObjectName("cardOfferBtn");

    m_toggleActiveBtn = new QPushButton(book.status == 1 ? "⏸ Set Inactive" : "▶ Set Active", m_overlay);
    m_toggleActiveBtn->setObjectName("cardToggleBtn");

    m_deleteBtn = new QPushButton("🗑 Delete", m_overlay);
    m_deleteBtn->setObjectName("cardDeleteBtn");

    QVBoxLayout *overlayLayout = new QVBoxLayout(m_overlay);
    overlayLayout->setContentsMargins(12, 12, 12, 12);
    overlayLayout->setSpacing(6);
    overlayLayout->addStretch();
    overlayLayout->addWidget(m_editBtn);
    overlayLayout->addWidget(m_offerBtn);
    overlayLayout->addWidget(m_toggleActiveBtn);
    overlayLayout->addWidget(m_deleteBtn);

    // 4. Book Title
    m_titleLabel = new QLabel(book.title, this);
    m_titleLabel->setObjectName("bookTitle");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setWordWrap(true);

    // 5. Main Card Layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    mainLayout->addWidget(coverFrame, 0, Qt::AlignCenter);
    mainLayout->addWidget(m_titleLabel, 1, Qt::AlignTop | Qt::AlignHCenter);

    // 6. Stylesheet
    setStyleSheet(R"(
        #bookCard {
            background-color: #9D0026;
            border-radius: 4px;
        }
        #coverFrame {
            background-color: #1F1724;
            border: 3px solid #A64D79;
            border-radius: 8px;
        }
        #bookCover {
            background-color: transparent;
            border: none;
            border-radius: 5px;
        }
        #bookTitle {
            color: #EAEAEA;
            font-size: 13px;
            font-weight: bold;
        }
        #cardOverlay {
            background-color: rgba(6, 5, 8, 210);
            border-radius: 5px;
        }
        #cardEditBtn, #cardOfferBtn, #cardToggleBtn, #cardDeleteBtn {
            border: none;
            border-radius: 6px;
            padding: 6px;
            font-size: 10px;
            font-weight: bold;
        }
        #cardEditBtn {
            background-color: #7C3E66;
            color: white;
        }
        #cardEditBtn:hover { background-color: #5F2E4F; }

        #cardOfferBtn {
            background-color: transparent;
            border: 1px solid #D4A017;
            color: #F0D48A;
        }
        #cardOfferBtn:hover { background-color: rgba(212, 160, 23, 60); color: white; }

        #cardToggleBtn {
            background-color: transparent;
            border: 1px solid #5B8DEF;
            color: #A9C3F5;
        }
        #cardToggleBtn:hover { background-color: rgba(91, 141, 239, 60); color: white; }

        #cardDeleteBtn {
            background-color: transparent;
            border: 1px solid #C0392B;
            color: #E6B0AA;
        }
        #cardDeleteBtn:hover { background-color: rgba(192, 57, 43, 60); color: white; }
    )");

    connect(m_editBtn, &QPushButton::clicked, this, [this]() { emit editRequested(m_book.id); });
    connect(m_deleteBtn, &QPushButton::clicked, this, [this]() { emit deleteRequested(m_book.id); });
    connect(m_offerBtn, &QPushButton::clicked, this, [this]() { emit offerRequested(m_book.id); });
    connect(m_toggleActiveBtn, &QPushButton::clicked, this, [this]() {
        int newStatus = (m_book.status == 1) ? 0 : 1;
        emit toggleActiveRequested(m_book.id, newStatus);
    });
}

int BookCardWidget::bookId() const
{
    return m_book.id;
}

void BookCardWidget::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    m_overlay->setVisible(true);
}

void BookCardWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_overlay->setVisible(false);
}
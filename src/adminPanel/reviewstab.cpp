#include "reviewstab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QJsonDocument>
#include <QColor>

ReviewsTab::ReviewsTab(QTcpSocket *socket, QWidget *parent)
    : QWidget(parent), m_socket(socket)
{
    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(setupUi());
}

QWidget* ReviewsTab::setupUi()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(12);

    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->setSpacing(10);

    m_reviewSearchEdit = new QLineEdit(page);
    m_reviewSearchEdit->setPlaceholderText("🔍 Search reviews by book title or username...");
    m_reviewSearchEdit->setStyleSheet(
        "QLineEdit { background-color: #120E14; border: 1px solid #1F1724; border-radius: 6px; "
        "padding: 8px; color: #EAEAEA; font-size: 13px; }"
        "QLineEdit:focus { border: 1px solid #7C3E66; }"
        );

    m_reviewFilterCombo = new QComboBox(page);
    m_reviewFilterCombo->addItem("All Reviews");
    m_reviewFilterCombo->addItem("Pending Approval");
    m_reviewFilterCombo->setStyleSheet(
        "QComboBox { background-color: #120E14; border: 1px solid #1F1724; border-radius: 6px; "
        "padding: 8px; color: #EAEAEA; font-size: 13px; min-width: 160px; }"
        "QComboBox::drop-down { border: none; }"
        );

    topBar->addWidget(m_reviewSearchEdit, 1);
    topBar->addWidget(m_reviewFilterCombo);
    layout->addLayout(topBar);

    m_reviewsTable = new QTableWidget(page);
    m_reviewsTable->setColumnCount(7);
    m_reviewsTable->setHorizontalHeaderLabels({"ID", "Book Title", "Username", "Rating", "Comment", "Date", "Status"});
    m_reviewsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_reviewsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_reviewsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_reviewsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_reviewsTable->verticalHeader()->setVisible(false);

    m_reviewsTable->setStyleSheet(
        "QTableWidget { background-color: #09070C; border: 1px solid #1F1724; gridline-color: #1F1724; color: #EAEAEA; }"
        "QTableWidget::item:selected { background-color: #7C3E66; color: white; }"
        "QHeaderView::section { background-color: #120E14; color: #A594B3; font-weight: bold; border: 1px solid #1F1724; padding: 6px; }"
        );
    layout->addWidget(m_reviewsTable);

    QHBoxLayout *reviewBtnLayout = new QHBoxLayout();
    reviewBtnLayout->setSpacing(10);

    m_btnApproveReview = new QPushButton("✅ Approve Review", page);
    m_btnApproveReview->setCursor(Qt::PointingHandCursor);
    m_btnApproveReview->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #268730; border-radius: 6px; "
        "padding: 8px; font-weight: bold; color: #7FDB8F; }"
        "QPushButton:hover { background-color: rgba(38, 135, 48, 50); color: white; }"
        );

    m_btnDeleteReview = new QPushButton("🗑️ Delete Review", page);
    m_btnDeleteReview->setCursor(Qt::PointingHandCursor);
    m_btnDeleteReview->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #C0392B; border-radius: 6px; padding: 8px; font-weight: bold; color: #E6B0AA; }"
        "QPushButton:hover { background-color: rgba(192, 57, 43, 50); color: white; }"
        );

    reviewBtnLayout->addStretch();
    reviewBtnLayout->addWidget(m_btnDeleteReview);
    reviewBtnLayout->addWidget(m_btnApproveReview);
    layout->addLayout(reviewBtnLayout);

    connect(m_reviewSearchEdit, &QLineEdit::textChanged, this, &ReviewsTab::filterReviews);
    connect(m_reviewFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ReviewsTab::handleReviewFilterChanged);
    connect(m_btnApproveReview, &QPushButton::clicked, this, &ReviewsTab::handleApproveReview);
    connect(m_btnDeleteReview, &QPushButton::clicked, this, &ReviewsTab::handleDeleteReview);

    return page;
}

void ReviewsTab::refreshTable()
{
    QJsonObject request;
    request["action"] = m_reviewShowPendingOnly ? "get_pending_reviews" : "get_all_reviews_admin";
    m_socket->write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");
    m_socket->flush();
}

void ReviewsTab::clearTableSelection()
{
    m_reviewsTable->clearSelection();
}

void ReviewsTab::handleReviewFilterChanged(int index)
{
    m_reviewShowPendingOnly = (index == 1);
    refreshTable();
}

void ReviewsTab::filterReviews(const QString &text)
{
    for (int i = 0; i < m_reviewsTable->rowCount(); ++i) {
        QString bookTitle = m_reviewsTable->item(i, 1)->text();
        QString username = m_reviewsTable->item(i, 2)->text();
        bool match = bookTitle.contains(text, Qt::CaseInsensitive) ||
                     username.contains(text, Qt::CaseInsensitive);
        m_reviewsTable->setRowHidden(i, !match);
    }
}

void ReviewsTab::populateReviewsTable(const QJsonArray &reviews, bool pendingOnly)
{
    m_reviewsTable->setRowCount(0);
    for (int i = 0; i < reviews.size(); ++i) {
        QJsonObject r = reviews[i].toObject();
        m_reviewsTable->insertRow(i);

        m_reviewsTable->setItem(i, 0, new QTableWidgetItem(QString::number(r["id"].toInt())));
        m_reviewsTable->setItem(i, 1, new QTableWidgetItem(r["bookTitle"].toString()));
        m_reviewsTable->setItem(i, 2, new QTableWidgetItem(r["username"].toString()));
        m_reviewsTable->setItem(i, 3, new QTableWidgetItem(QString::number(r["rating"].toInt()) + " ⭐"));
        m_reviewsTable->setItem(i, 4, new QTableWidgetItem(r["comment"].toString()));
        m_reviewsTable->setItem(i, 5, new QTableWidgetItem(r["date"].toString()));

        bool isApproved = pendingOnly ? false : r["isApproved"].toBool();
        QTableWidgetItem *statusItem = new QTableWidgetItem(isApproved ? "✅ Approved" : "⏳ Pending");
        statusItem->setForeground(isApproved ? QColor("#7FDB8F") : QColor("#F5D7A0"));
        m_reviewsTable->setItem(i, 6, statusItem);

        m_reviewsTable->item(i, 0)->setData(Qt::UserRole, r["bookId"].toInt());
        m_reviewsTable->item(i, 2)->setData(Qt::UserRole, r["userId"].toInt());
    }
}

void ReviewsTab::handleApproveReview()
{
    int currentRow = m_reviewsTable->currentRow();
    if (currentRow < 0) return;

    int reviewId = m_reviewsTable->item(currentRow, 0)->text().toInt();

    QJsonObject packet;
    packet["action"] = "approve_review";
    packet["reviewId"] = reviewId;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void ReviewsTab::handleDeleteReview()
{
    int currentRow = m_reviewsTable->currentRow();
    if (currentRow < 0) return;

    int reviewId = m_reviewsTable->item(currentRow, 0)->text().toInt();
    QString bookTitle = m_reviewsTable->item(currentRow, 1)->text();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirm Delete",
        "Are you sure you want to remove this review for \"" + bookTitle + "\"?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QJsonObject packet;
        packet["action"] = "review_delete";
        packet["reviewId"] = reviewId;
        m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
    }
}

void ReviewsTab::handleServerResponse(const QJsonObject &response)
{
    QString action = response["action"].toString();

    if (action == "all_reviews_response" && response["status"].toString() == "success") {
        populateReviewsTable(response["data"].toArray(), false);
    }
    else if (action == "pending_reviews_response" && response["status"].toString() == "success") {
        populateReviewsTable(response["data"].toArray(), true);
    }
    else if (action == "approve_review_response") {
        if (response["status"].toString() == "success") {
            refreshTable();
        } else {
            QMessageBox::warning(this, "Approve Failed", response["message"].toString());
        }
    }
    else if (action == "review_delete_response") {
        if (response["status"].toString() == "success") {
            QMessageBox::information(this, "Success", "Review removed successfully.");
            refreshTable();
        } else {
            QMessageBox::warning(this, "Delete Failed", response["message"].toString());
        }
    }
}

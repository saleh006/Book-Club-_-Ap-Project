#include "publisherstab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFormLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>

PublishersTab::PublishersTab(QTcpSocket *socket, QWidget *parent)
    : QWidget(parent), m_socket(socket)
{
    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(setupUi());
}

void PublishersTab::setRowDimmed(QTableWidget *table, int row, bool isDimmed)
{
    for (int col = 0; col < table->columnCount(); ++col) {
        QTableWidgetItem *item = table->item(row, col);
        if (item) {
            if (isDimmed) {
                item->setForeground(QColor("#6A5D75"));
                item->setBackground(QColor("#050407"));
            } else {
                item->setForeground(QColor("#EAEAEA"));
                item->setBackground(QColor("#09070C"));
            }
        }
    }
}

QWidget* PublishersTab::setupUi()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(12);

    m_publisherSearchEdit = new QLineEdit(page);
    m_publisherSearchEdit->setPlaceholderText("🔍 Search publishers by username or name ...");
    m_publisherSearchEdit->setStyleSheet(
        "QLineEdit { background-color: #120E14; border: 1px solid #1F1724; border-radius: 6px; "
        "padding: 8px; color: #EAEAEA; font-size: 13px; }"
        "QLineEdit:focus { border: 1px solid #7C3E66; }"
        );
    layout->addWidget(m_publisherSearchEdit);

    m_publishersTable = new QTableWidget(page);
    m_publishersTable->setColumnCount(6);
    m_publishersTable->setHorizontalHeaderLabels({"ID", "Username", "Publisher Name", "Books", "Status", "Register Date"});
    m_publishersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_publishersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_publishersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_publishersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_publishersTable->verticalHeader()->setVisible(false);
    m_publishersTable->setStyleSheet(
        "QTableWidget { background-color: #09070C; border: 1px solid #1F1724; gridline-color: #1F1724; color: #EAEAEA; }"
        "QTableWidget::item:selected { background-color: #7C3E66; color: white; }"
        "QHeaderView::section { background-color: #120E14; color: #A594B3; font-weight: bold; border: 1px solid #1F1724; padding: 6px; }"
        );
    layout->addWidget(m_publishersTable);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_btnBlockPublisher = new QPushButton("🚫 Block Publisher", page);
    m_btnUnblockPublisher = new QPushButton("✅ Unblock Publisher", page);
    m_btnPublisherDetails = new QPushButton("👁 View Details", page);
    m_btnDeletePublisher = new QPushButton("🗑️ Delete Publisher", page);

    m_btnBlockPublisher->setCursor(Qt::PointingHandCursor);
    m_btnUnblockPublisher->setCursor(Qt::PointingHandCursor);
    m_btnPublisherDetails->setCursor(Qt::PointingHandCursor);
    m_btnDeletePublisher->setCursor(Qt::PointingHandCursor);

    QString blockStyle =
        "QPushButton { background-color: transparent; border: 1px solid #C0392B; border-radius: 6px; padding: 8px; font-weight: bold; color: #E6B0AA; }"
        "QPushButton:hover { background-color: rgba(192, 57, 43, 50); color: white; }";
    QString unblockStyle =
        "QPushButton { background-color: transparent; border: 1px solid #2ECC71; border-radius: 6px; padding: 8px; font-weight: bold; color: #ABEBC6; }"
        "QPushButton:hover { background-color: rgba(46, 204, 113, 50); color: white; }";
    QString detailsStyle =
        "QPushButton { background-color: transparent; border: 1px solid #3498DB; border-radius: 6px; padding: 8px; font-weight: bold; color: #AED6F1; }"
        "QPushButton:hover { background-color: rgba(52, 152, 219, 50); color: white; }";
    QString deleteStyle =
        "QPushButton { background-color: transparent; border: 1px solid #C0392B; border-radius: 6px; padding: 8px; font-weight: bold; color: #E6B0AA; }"
        "QPushButton:hover { background-color: rgba(192, 57, 43, 50); color: white; }";

    m_btnBlockPublisher->setStyleSheet(blockStyle);
    m_btnUnblockPublisher->setStyleSheet(unblockStyle);
    m_btnPublisherDetails->setStyleSheet(detailsStyle);
    m_btnDeletePublisher->setStyleSheet(deleteStyle);

    btnLayout->addWidget(m_btnBlockPublisher);
    btnLayout->addWidget(m_btnUnblockPublisher);
    btnLayout->addWidget(m_btnDeletePublisher);
    btnLayout->addWidget(m_btnPublisherDetails);
    layout->addLayout(btnLayout);

    connect(m_publisherSearchEdit, &QLineEdit::textChanged, this, &PublishersTab::filterPublishers);
    connect(m_btnBlockPublisher, &QPushButton::clicked, this, &PublishersTab::handleBlockPublisher);
    connect(m_btnUnblockPublisher, &QPushButton::clicked, this, &PublishersTab::handleUnblockPublisher);
    connect(m_btnPublisherDetails, &QPushButton::clicked, this, &PublishersTab::handleViewPublisherDetails);
    connect(m_btnDeletePublisher, &QPushButton::clicked, this, &PublishersTab::handleDeletePublisher);

    return page;
}

void PublishersTab::refreshTable()
{
    QJsonObject request;
    request["action"] = "get_publishers_list";
    m_socket->write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");
    m_socket->flush();
}

void PublishersTab::clearTableSelection()
{
    m_publishersTable->clearSelection();
}

void PublishersTab::filterPublishers(const QString &text)
{
    for (int i = 0; i < m_publishersTable->rowCount(); ++i) {
        bool match = false;
        if (m_publishersTable->item(i, 1)->text().contains(text, Qt::CaseInsensitive) ||
            m_publishersTable->item(i, 2)->text().contains(text, Qt::CaseInsensitive)) {
            match = true;
        }
        m_publishersTable->setRowHidden(i, !match);
    }
}

void PublishersTab::handleBlockPublisher()
{
    int currentRow = m_publishersTable->currentRow();
    if (currentRow < 0) return;

    QString username = m_publishersTable->item(currentRow, 1)->text();
    QJsonObject packet;
    packet["action"] = "set_user_block_status";
    packet["username"] = username;
    packet["block_status"] = true;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void PublishersTab::handleUnblockPublisher()
{
    int currentRow = m_publishersTable->currentRow();
    if (currentRow < 0) return;

    QString username = m_publishersTable->item(currentRow, 1)->text();
    QJsonObject packet;
    packet["action"] = "set_user_block_status";
    packet["username"] = username;
    packet["block_status"] = false;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void PublishersTab::handleViewPublisherDetails()
{
    int currentRow = m_publishersTable->currentRow();
    if (currentRow < 0) return;

    QString username = m_publishersTable->item(currentRow, 1)->text();

    QJsonObject packet;
    packet["action"] = "get_publisher_details";
    packet["username"] = username;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void PublishersTab::handleDeletePublisher()
{
    int currentRow = m_publishersTable->currentRow();
    if (currentRow < 0) return;

    QString username = m_publishersTable->item(currentRow, 1)->text();
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Delete",
                                                              "Are you sure you want to completely delete publisher: " + username + "?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QJsonObject packet;
        packet["action"] = "delete_account";
        packet["username"] = username;
        m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
    }
}

void PublishersTab::handleServerResponse(const QJsonObject &response)
{
    QString action = response["action"].toString();

    if (response.contains("type") && response["type"].toString() == "table_refresh_required") {
        if (response["target_table"].toString() == "publishers") {
            refreshTable();
        }
        return;
    }

    if (action == "publishers_list_response" && response["status"] == "success") {
        QJsonArray publishers = response["data"].toArray();
        m_publishersTable->setRowCount(0);

        for (int i = 0; i < publishers.size(); ++i) {
            QJsonObject p = publishers[i].toObject();
            m_publishersTable->insertRow(i);

            m_publishersTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
            m_publishersTable->setItem(i, 1, new QTableWidgetItem(p["username"].toString()));
            m_publishersTable->setItem(i, 2, new QTableWidgetItem(p["fullName"].toString()));
            m_publishersTable->setItem(i, 3, new QTableWidgetItem(QString::number(p["publishedBooksCount"].toInt())));
            bool isBlocked = p["isBlocked"].toBool();
            m_publishersTable->setItem(i, 4, new QTableWidgetItem(isBlocked ? "Blocked" : "Active"));
            m_publishersTable->setItem(i, 5, new QTableWidgetItem(p["registerDate"].toString()));

            if (isBlocked) setRowDimmed(m_publishersTable, i, true);
        }
    }
    else if (action == "set_user_block_status_response" && response["status"] == "success") {
        bool isBlocked = response["block_status"].toBool();
        QString targetUser = response["username"].toString();

        for (int i = 0; i < m_publishersTable->rowCount(); i++) {
            if (m_publishersTable->item(i, 1)->text() == targetUser) {
                m_publishersTable->item(i, 4)->setText(isBlocked ? "Blocked" : "Active");
                setRowDimmed(m_publishersTable, i, isBlocked);
                break;
            }
        }
    }
    else if (action == "publisher_details_response" && response["status"] == "success") {
        showPublisherDetailsDialog(response["data"].toObject());
    }
    else if (action == "delete_account_response" && response["status"] == "success") {
        refreshTable();
    }
}

void PublishersTab::showPublisherDetailsDialog(const QJsonObject &data)
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Publisher Profile Details");
    dialog->resize(640,680);
    dialog->setStyleSheet("background-color: #09070C; color: #EAEAEA; font-family: 'Segoe UI', Arial;");

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    mainLayout->setContentsMargins(24,24,24,20);
    mainLayout->setSpacing(14);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(16);

    QLabel *avatar = new QLabel("🧑‍💻", dialog);
    avatar->setFixedSize(64,64);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet("font-size: 28px; background-color: #1F1724; border-radius: 32px;");

    QVBoxLayout *headerTextLayout = new QVBoxLayout();
    headerTextLayout->setSpacing(2);

    QLabel *nameLabel = new QLabel(data["fullName"].toString(), dialog);
    nameLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #FFFFFF;");

    QLabel *usernameLabel = new QLabel("@" + data["username"].toString(), dialog);
    usernameLabel->setStyleSheet("font-size: 12px; color: #9A8FA0;");

    QHBoxLayout *badgeRow = new QHBoxLayout();
    badgeRow->setSpacing(8);

    QLabel *roleBadge = new QLabel("PUBLISHER PARTNER", dialog);
    roleBadge->setStyleSheet(
        "background-color: rgba(124, 62, 102, 90); color: #FFEAD2; font-size: 10px; font-weight: bold; "
        "padding: 3px 10px; border-radius: 8px; letter-spacing: 1px;");
    roleBadge->setFixedHeight(20);

    bool isBlocked = data["isBlocked"].toBool();
    QLabel *statusBadge = new QLabel(isBlocked ? "🚫 SUSPENDED" : "✅ ACTIVE", dialog);
    statusBadge->setStyleSheet(
        QString("background-color: %1; color: white; font-size: 10px; font-weight: bold; "
                "padding: 3px 10px; border-radius: 8px; letter-spacing: 1px;")
            .arg(isBlocked ? "rgba(192,57,43,180)" : "rgba(46,204,113,150)"));
    statusBadge->setFixedHeight(20);

    badgeRow->addWidget(roleBadge);
    badgeRow->addWidget(statusBadge);
    badgeRow->addStretch();

    headerTextLayout->addWidget(nameLabel);
    headerTextLayout->addWidget(usernameLabel);
    headerTextLayout->addSpacing(4);
    headerTextLayout->addLayout(badgeRow);

    headerLayout->addWidget(avatar);
    headerLayout->addLayout(headerTextLayout, 1);
    mainLayout->addLayout(headerLayout);

    QString cardStyle =
        "QGroupBox { border: 1px solid #1F1724; border-radius: 10px; margin-top: 8px; padding-top: 16px; "
        "color: #A594B3; font-weight: bold; background-color: #0F0C12; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }";

    QGroupBox *infoBox = new QGroupBox("Account Information", dialog);
    infoBox->setStyleSheet(cardStyle);
    QFormLayout *formLayout = new QFormLayout(infoBox);
    formLayout->setLabelAlignment(Qt::AlignRight);
    formLayout->setHorizontalSpacing(16);
    formLayout->setVerticalSpacing(8);

    QString labelStyle = "color: #9A8FA0; font-size: 13px;";
    QString valueStyle = "color: #EAEAEA; font-size: 14px; font-weight: bold;";

    auto addRow = [&](const QString &title, const QString &value) {
        QLabel *lbl = new QLabel(title);
        lbl->setStyleSheet(labelStyle);
        QLabel *val = new QLabel(value.isEmpty() ? "_" : value);
        val->setStyleSheet(valueStyle);
        formLayout->addRow(lbl, val);
    };

    addRow("Email:", data["email"].toString());
    addRow("Registered:", data["registerDate"].toString());

    mainLayout->addWidget(infoBox);

    QGroupBox *statsBox = new QGroupBox("Performance Dashboard", dialog);
    statsBox->setStyleSheet(cardStyle);
    QGridLayout *statsLayout = new QGridLayout(statsBox);
    statsLayout->setHorizontalSpacing(10);
    statsLayout->setVerticalSpacing(10);

    auto createStatWidget = [&](const QString &title, const QString &valueText, const QString &color) {
        QWidget *w = new QWidget();
        w->setStyleSheet("background-color: #120E14; border-radius: 8px;");
        QVBoxLayout *vl = new QVBoxLayout(w);
        vl->setContentsMargins(8, 10, 8, 10);
        QLabel *c = new QLabel(valueText);
        c->setAlignment(Qt::AlignCenter);
        c->setStyleSheet(QString("font-size: 20px; color: %1; font-weight: bold; border: none; background: transparent;").arg(color));
        QLabel *t = new QLabel(title);
        t->setAlignment(Qt::AlignCenter);
        t->setStyleSheet("font-size: 10px; color: #9A8FA0; border: none; background: transparent;");
        vl->addWidget(c);
        vl->addWidget(t);
        return w;
    };

    QString booksCount = QString::number(data["publishedBooksCount"].toInt());
    QString salesCount = QString::number(data["totalSales"].toInt());

    double avgRating = data["averageRating"].toDouble();
    QString ratingStr = avgRating > 0.0 ? QString::number(avgRating, 'f', 1) + " ⭐" : "N/A";

    double income = data["totalIncome"].toDouble();
    QString incomeStr = "$" + QString::number(income, 'f', 2);

    statsLayout->addWidget(createStatWidget("Total Books", booksCount, "#DDA0DD"), 0, 0);
    statsLayout->addWidget(createStatWidget("Total Sales", salesCount, "#3498DB"), 0, 1);
    statsLayout->addWidget(createStatWidget("Avg Rating", ratingStr, "#F1C40F"), 0, 2);
    statsLayout->addWidget(createStatWidget("Est. Income", incomeStr, "#2ECC71"), 0, 3);

    mainLayout->addWidget(statsBox);

    QGroupBox *booksBox = new QGroupBox("Published Catalog", dialog);
    booksBox->setStyleSheet(cardStyle);
    QVBoxLayout *booksBoxLayout = new QVBoxLayout(booksBox);

    QJsonArray booksArr = data["publishedBooks"].toArray();
    QTableWidget *booksTable = new QTableWidget(booksArr.size(), 5, dialog);
    booksTable->setHorizontalHeaderLabels({"Title", "Genre", "Price", "Sales", "Rating"});
    booksTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    booksTable->verticalHeader()->setVisible(false);
    booksTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    booksTable->setSelectionMode(QAbstractItemView::NoSelection);
    booksTable->setStyleSheet(
        "QTableWidget { background-color: #09070C; border: 1px solid #1F1724; gridline-color: #1F1724; color: #EAEAEA; }"
        "QTableWidget::item { padding: 4px; }"
        "QHeaderView::section { background-color: #120E14; color: #A594B3; font-weight: bold; border: 1px solid #1F1724; padding: 6px; }");

    for (int i = 0; i < booksArr.size(); ++i) {
        QJsonObject b = booksArr[i].toObject();
        bool active = b["isActive"].toBool();

        QTableWidgetItem *titleItem = new QTableWidgetItem(b["title"].toString());
        if (!active) titleItem->setForeground(QColor("#6B6470"));
        booksTable->setItem(i, 0, titleItem);

        booksTable->setItem(i, 1, new QTableWidgetItem(b["genre"].toString()));
        booksTable->setItem(i, 2, new QTableWidgetItem("$" + QString::number(b["price"].toDouble(), 'f', 2)));
        booksTable->setItem(i, 3, new QTableWidgetItem(QString::number(b["totalSales"].toInt())));

        double r = b["averageRating"].toDouble();
        booksTable->setItem(i, 4, new QTableWidgetItem(r > 0.0 ? QString::number(r, 'f', 1) + " ⭐" : "—"));
    }
    if (booksArr.isEmpty()) {
        booksTable->setRowCount(1);
        booksTable->setSpan(0, 0, 1, 5);
        QTableWidgetItem *empty = new QTableWidgetItem("No books published yet.");
        empty->setTextAlignment(Qt::AlignCenter);
        empty->setForeground(QColor("#6B6470"));
        booksTable->setItem(0, 0, empty);
    }

    booksBoxLayout->addWidget(booksTable);
    mainLayout->addWidget(booksBox, 1);

    QPushButton *closeBtn = new QPushButton("Close", dialog);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #1F1724; border: none; border-radius: 6px; padding: 10px; color: white; font-weight: bold; }"
        "QPushButton:hover { background-color: #7C3E66; }"
        );
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);

    QHBoxLayout *footerLayout = new QHBoxLayout();
    footerLayout->addStretch();
    footerLayout->addWidget(closeBtn);
    mainLayout->addLayout(footerLayout);

    dialog->exec();
    dialog->deleteLater();
}

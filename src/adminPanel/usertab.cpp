#include "usertab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFormLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>

UsersTab::UsersTab(QTcpSocket *socket, QWidget *parent)
    : QWidget(parent), m_socket(socket)
{
    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(setupUi());
}

void UsersTab::setRowDimmed(QTableWidget *table, int row, bool isDimmed)
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

QWidget* UsersTab::setupUi()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(12);

    m_searchEdit = new QLineEdit(page);
    m_searchEdit->setPlaceholderText("🔍 Search users by username ...");
    m_searchEdit->setStyleSheet(
        "QLineEdit { background-color: #120E14; border: 1px solid #1F1724; border-radius: 6px; "
        "padding: 8px; color: #EAEAEA; font-size: 13px; }"
        "QLineEdit:focus { border: 1px solid #7C3E66; }"
        );
    layout->addWidget(m_searchEdit);

    m_usersTable = new QTableWidget(page);
    m_usersTable->setColumnCount(5);
    m_usersTable->setHorizontalHeaderLabels({"ID", "Username", "Role", "Status" , "Register Date"});
    m_usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_usersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_usersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_usersTable->verticalHeader()->setVisible(false);

    m_usersTable->setStyleSheet(
        "QTableWidget { background-color: #09070C; border: 1px solid #1F1724; gridline-color: #1F1724; color: #EAEAEA; }"
        "QTableWidget::item:selected { background-color: #7C3E66; color: white; }"
        "QHeaderView::section { background-color: #120E14; color: #A594B3; font-weight: bold; border: 1px solid #1F1724; padding: 6px; }"
        );
    layout->addWidget(m_usersTable);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_btnBlock = new QPushButton("🚫 Block User", page);
    m_btnUnblock = new QPushButton("✅ Unblock User", page);
    m_btnUserDetails = new QPushButton("👁 View Details", page);
    m_btnDeleteUser = new QPushButton("🗑️ Delete User", page);

    m_btnBlock->setCursor(Qt::PointingHandCursor);
    m_btnUnblock->setCursor(Qt::PointingHandCursor);
    m_btnUserDetails->setCursor(Qt::PointingHandCursor);
    m_btnDeleteUser->setCursor(Qt::PointingHandCursor);

    m_btnBlock->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #C0392B; border-radius: 6px; padding: 8px; font-weight: bold; color: #E6B0AA; }"
        "QPushButton:hover { background-color: rgba(192, 57, 43, 50); color: white; }"
        );
    m_btnUnblock->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #2ECC71; border-radius: 6px; padding: 8px; font-weight: bold; color: #ABEBC6; }"
        "QPushButton:hover { background-color: rgba(46, 204, 113, 50); color: white; }"
        );
    m_btnUserDetails->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #3498DB; border-radius: 6px; padding: 8px; font-weight: bold; color: #AED6F1; }"
        "QPushButton:hover { background-color: rgba(52, 152, 219, 50); color: white; }"
        );
    m_btnDeleteUser->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #C0392B; border-radius: 6px; padding: 8px; font-weight: bold; color: #E6B0AA; }"
        "QPushButton:hover { background-color: rgba(192, 57, 43, 50); color: white; }"
        );

    btnLayout->addWidget(m_btnBlock);
    btnLayout->addWidget(m_btnUnblock);
    btnLayout->addWidget(m_btnDeleteUser);
    btnLayout->addWidget(m_btnUserDetails);
    layout->addLayout(btnLayout);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &UsersTab::filterUsers);
    connect(m_btnBlock, &QPushButton::clicked, this, &UsersTab::handleBlockUser);
    connect(m_btnUnblock, &QPushButton::clicked, this, &UsersTab::handleUnblockUser);
    connect(m_btnUserDetails, &QPushButton::clicked, this, &UsersTab::handleViewUserDetails);
    connect(m_btnDeleteUser, &QPushButton::clicked, this, &UsersTab::handleDeleteUser);

    return page;
}

void UsersTab::refreshTable()
{
    QJsonObject request;
    request["action"] = "get_users_list";
    m_socket->write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");
    m_socket->flush();
}

void UsersTab::clearTableSelection()
{
    m_usersTable->clearSelection();
}

void UsersTab::filterUsers(const QString &text)
{
    for (int i = 0; i < m_usersTable->rowCount(); ++i) {
        bool match = false;
        if (m_usersTable->item(i, 1)->text().contains(text, Qt::CaseInsensitive)) {
            match = true;
        }
        m_usersTable->setRowHidden(i, !match);
    }
}

void UsersTab::handleBlockUser()
{
    int currentRow = m_usersTable->currentRow();
    if (currentRow < 0) return;

    QString username = m_usersTable->item(currentRow, 1)->text();
    QJsonObject packet;
    packet["action"] = "set_user_block_status";
    packet["username"] = username;
    packet["block_status"] = true;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void UsersTab::handleUnblockUser()
{
    int currentRow = m_usersTable->currentRow();
    if (currentRow < 0) return;

    QString username = m_usersTable->item(currentRow, 1)->text();
    QJsonObject packet;
    packet["action"] = "set_user_block_status";
    packet["username"] = username;
    packet["block_status"] = false;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void UsersTab::handleViewUserDetails()
{
    int currentRow = m_usersTable->currentRow();
    if (currentRow < 0) return;

    QString username = m_usersTable->item(currentRow, 1)->text();

    QJsonObject packet;
    packet["action"] = "get_user_details";
    packet["username"] = username;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void UsersTab::handleDeleteUser()
{
    int currentRow = m_usersTable->currentRow();
    if (currentRow < 0) return;

    QString username = m_usersTable->item(currentRow, 1)->text();
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Delete",
                                                              "Are you sure you want to completely delete user: " + username + "?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QJsonObject packet;
        packet["action"] = "delete_account";
        packet["username"] = username;
        m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
    }
}

void UsersTab::handleServerResponse(const QJsonObject &response)
{
    QString action = response["action"].toString();

    if (response.contains("type") && response["type"].toString() == "table_refresh_required") {
        if (response["target_table"].toString() == "users") {
            refreshTable();
        }
        return;
    }

    if (action == "users_list_response" && response["status"] == "success") {
        QJsonArray users = response["data"].toArray();
        m_usersTable->setRowCount(0);

        for (int i = 0; i < users.size(); ++i) {
            QJsonObject u = users[i].toObject();
            m_usersTable->insertRow(i);

            m_usersTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
            m_usersTable->setItem(i, 1, new QTableWidgetItem(u["username"].toString()));
            m_usersTable->setItem(i, 2, new QTableWidgetItem(u["role"].toString()));
            bool isBlocked = u["isBlocked"].toBool();
            m_usersTable->setItem(i, 3, new QTableWidgetItem(isBlocked ? "Blocked" : "Active"));
            m_usersTable->setItem(i, 4, new QTableWidgetItem(u["registerDate"].toString()));

            if (isBlocked) setRowDimmed(m_usersTable, i, true);
        }
    }
    else if (action == "set_user_block_status_response" && response["status"] == "success") {
        bool isBlocked = response["block_status"].toBool();
        QString targetUser = response["username"].toString();

        for (int i = 0; i < m_usersTable->rowCount(); i++) {
            if (m_usersTable->item(i, 1)->text() == targetUser) {
                m_usersTable->item(i, 3)->setText(isBlocked ? "Blocked" : "Active");
                setRowDimmed(m_usersTable, i, isBlocked);
                break;
            }
        }
    }
    else if (action == "user_details_response" && response["status"] == "success") {
        showUserDetailsDialog(response["data"].toObject());
    }
    else if (action == "delete_account_response" && response["status"] == "success") {
        QMessageBox::information(this, "Success", "Account deleted successfully from the database.");
        refreshTable();
    }
}

void UsersTab::showUserDetailsDialog(const QJsonObject &data)
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("User Profile Details");
    dialog->resize(620,680);
    dialog->setStyleSheet("background-color: #09070C; color: #EAEAEA; font-family: 'Segoe UI', Arial;");

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    mainLayout->setContentsMargins(24,24,24,20);
    mainLayout->setSpacing(14);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(16);

    QLabel *avatar = new QLabel("👤", dialog);
    avatar->setFixedSize(64,64);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet("font-size: 30px; background-color: #1F1724; border-radius: 32px; color: #DDA0DD;");

    QVBoxLayout *headerTextLayout = new QVBoxLayout();
    headerTextLayout->setSpacing(2);

    QLabel *nameLabel = new QLabel(data["fullName"].toString(), dialog);
    nameLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #FFFFFF;");

    QLabel *usernameLabel = new QLabel("@" + data["username"].toString(), dialog);
    usernameLabel->setStyleSheet("font-size: 12px; color: #9A8FA0;");

    QHBoxLayout *badgeRow = new QHBoxLayout();
    badgeRow->setSpacing(8);

    QLabel *roleBadge = new QLabel(data["role"].toString().toUpper(), dialog);
    roleBadge->setStyleSheet(
        "background-color: #5C2E4C; color: #FFEAD2; font-size: 10px; font-weight: bold; "
        "padding: 3px 10px; border-radius: 8px; letter-spacing: 1px;");
    roleBadge->setFixedHeight(20);

    bool isBlocked = data["isBlocked"].toBool();
    QLabel *statusBadge = new QLabel(isBlocked ? "🚫 BLOCKED" : "✅ ACTIVE", dialog);
    statusBadge->setStyleSheet(
        QString("background-color: %1; color: white; font-size: 10px; font-weight: bold; "
                "padding: 3px 10px; border-radius: 8px; letter-spacing: 1px;")
            .arg(isBlocked ? "#8d1f1f" : "#268730"));
    statusBadge->setFixedHeight(20);

    badgeRow->addWidget(roleBadge);
    badgeRow->addWidget(statusBadge);
    badgeRow->addStretch();

    headerTextLayout->addWidget(nameLabel);
    headerTextLayout->addWidget(usernameLabel);
    headerTextLayout->addSpacing(4);
    headerTextLayout->addLayout(badgeRow);

    headerLayout->addWidget(avatar);
    headerLayout->addLayout(headerTextLayout , 1);
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

    QString labelStyle = "color: #9A8FA0; font-size: 12px; border: none; background: transparent;";
    QString valueStyle = "color: #EAEAEA; font-size: 13px; font-weight: 600; border: none; background: transparent;";

    auto addRow = [&](const QString &title, const QString &value) {
        QLabel *lbl = new QLabel(title);
        lbl->setStyleSheet(labelStyle);
        QLabel *val = new QLabel(value.isEmpty() ? "_" : value);
        val->setStyleSheet(valueStyle);
        formLayout->addRow(lbl, val);
    };

    addRow("Email:", data["email"].toString());
    addRow("Registered:", data["registerDate"].toString());
    addRow("Card Items:",QString::number(data["cartItemCount"].toInt()));
    addRow("Cart Total:", "$" + QString::number(data["cartTotal"].toDouble(), 'f', 2));

    mainLayout->addWidget(infoBox);

    QGroupBox *statsBox = new QGroupBox("Library Activity", dialog);
    statsBox->setStyleSheet(cardStyle);
    QHBoxLayout *statsLayout = new QHBoxLayout(statsBox);

    auto createStatWidget = [&](const QString &title, const QString &valueText, const QString &color) {
        QWidget *w = new QWidget();
        w->setStyleSheet("background-color: #120E14; border-radius: 8px;");
        QVBoxLayout *vl = new QVBoxLayout(w);
        vl->setContentsMargins(8,10,8,10);
        QLabel *c = new QLabel(valueText);
        c->setAlignment(Qt::AlignCenter);
        c->setStyleSheet(QString("font-size: 22px; color: %1; font-weight: bold; borde: none; backgroud: transparent").arg(color));
        QLabel *t = new QLabel(title);
        t->setAlignment(Qt::AlignCenter);
        t->setStyleSheet("font-size: 11px; color: #9A8FA0; border: none; background: transparent;");
        vl->addWidget(c);
        vl->addWidget(t);
        return w;
    };

    statsLayout->addWidget(createStatWidget("Owned Books", QString::number(data["ownedBooksCount"].toInt()), "#7C3E66"));
    statsLayout->addWidget(createStatWidget("Wishlist", QString::number(data["wishlistCount"].toInt()), "#DDA0DD"));
    statsLayout->addWidget(createStatWidget("Purchases", QString::number(data["totalPurchases"].toInt()), "#3498DB"));
    statsLayout->addWidget(createStatWidget("Total Spent", "$" + QString::number(data["totalSpent"].toDouble(), 'f', 2), "#2ECC71"));

    mainLayout->addWidget(statsBox);

    QTabWidget *tabs = new QTabWidget(dialog);
    tabs->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #1F1724; border-radius: 8px; background-color: #09070C; top: -1px; }"
        "QTabBar::tab { background-color: #120E14; color: #9A8FA0; padding: 8px 16px; border: 1px solid #1F1724; "
        "border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; font-size: 12px; }"
        "QTabBar::tab:selected { background-color: #1F1724; color: #EAEAEA; font-weight: bold; }"
        "QTabBar::tab:hover { color: #EAEAEA; }");

    QString tableStyle =
        "QTableWidget { background-color: #09070C; border: none; gridline-color: #1F1724; color: #EAEAEA; }"
        "QTableWidget::item { padding: 4px; }"
        "QTableWidget::item:selected { background-color: #7C3E66; color: white; }"
        "QHeaderView::section { background-color: #120E14; color: #A594B3; font-weight: bold; border: 1px solid #1F1724; padding: 6px; }";

    auto makeEmptyRow = [](QTableWidget *table, int columnSpan, const QString &message) {
        table->setRowCount(1);
        table->setSpan(0, 0, 1, columnSpan);
        QTableWidgetItem *empty = new QTableWidgetItem(message);
        empty->setTextAlignment(Qt::AlignCenter);
        empty->setForeground(QColor("#6B6470"));
        table->setItem(0, 0, empty);
    };

    QJsonArray ownedArr = data["ownedBooks"].toArray();
    QTableWidget *ownedTable = new QTableWidget(ownedArr.size(), 2, dialog);
    ownedTable->setHorizontalHeaderLabels({"Title", "Author"});
    ownedTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ownedTable->verticalHeader()->setVisible(false);
    ownedTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ownedTable->setSelectionMode(QAbstractItemView::NoSelection);
    ownedTable->setStyleSheet(tableStyle);
    for (int i = 0; i < ownedArr.size(); ++i) {
        QJsonObject b = ownedArr[i].toObject();
        ownedTable->setItem(i, 0, new QTableWidgetItem(b["title"].toString()));
        ownedTable->setItem(i, 1, new QTableWidgetItem(b["author"].toString()));
    }
    if (ownedArr.isEmpty()) makeEmptyRow(ownedTable, 2, "No books owned yet.");
    tabs->addTab(ownedTable, QString("📚 Owned (%1)").arg(data["ownedBooksCount"].toInt()));

    QJsonArray wishlistArr = data["wishlist"].toArray();
    QTableWidget *wishTable = new QTableWidget(wishlistArr.size(), 2, dialog);
    wishTable->setHorizontalHeaderLabels({"Title", "Author"});
    wishTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    wishTable->verticalHeader()->setVisible(false);
    wishTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    wishTable->setSelectionMode(QAbstractItemView::NoSelection);
    wishTable->setStyleSheet(tableStyle);
    for (int i = 0; i < wishlistArr.size(); ++i) {
        QJsonObject b = wishlistArr[i].toObject();
        wishTable->setItem(i, 0, new QTableWidgetItem(b["title"].toString()));
        wishTable->setItem(i, 1, new QTableWidgetItem(b["author"].toString()));
    }
    if (wishlistArr.isEmpty()) makeEmptyRow(wishTable, 2, "Wishlist is empty.");
    tabs->addTab(wishTable, QString("💜 Wishlist (%1)").arg(data["wishlistCount"].toInt()));

    QJsonArray purchasesArr = data["purchaseHistory"].toArray();
    QTableWidget *purchTable = new QTableWidget(purchasesArr.size(), 3, dialog);
    purchTable->setHorizontalHeaderLabels({"Date", "Items", "Total"});
    purchTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    purchTable->verticalHeader()->setVisible(false);
    purchTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    purchTable->setSelectionMode(QAbstractItemView::NoSelection);
    purchTable->setStyleSheet(tableStyle);
    for (int i = 0; i < purchasesArr.size(); ++i) {
        QJsonObject p = purchasesArr[i].toObject();
        purchTable->setItem(i, 0, new QTableWidgetItem(p["date"].toString()));
        purchTable->setItem(i, 1, new QTableWidgetItem(QString::number(p["itemCount"].toInt())));
        purchTable->setItem(i, 2, new QTableWidgetItem("$" + QString::number(p["total"].toDouble(), 'f', 2)));
    }
    if (purchasesArr.isEmpty()) makeEmptyRow(purchTable, 3, "No purchases yet.");
    tabs->addTab(purchTable, QString("🧾 Purchases (%1)").arg(data["totalPurchases"].toInt()));

    mainLayout->addWidget(tabs, 1);

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
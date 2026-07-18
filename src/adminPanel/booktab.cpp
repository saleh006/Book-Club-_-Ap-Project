#include "src/adminPanel/booktab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMessageBox>

static bool uploadFileToServer(const QString &localFilePath, const QString &fileType,
                               QString &outServerPath, QString &errorMsg)
{
    QFile file(localFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        errorMsg = "Could not open file: " + localFilePath;
        return false;
    }
    QByteArray fileBytes = file.readAll();
    file.close();

    QFileInfo info(localFilePath);

    QJsonObject req;
    req["action"] = "upload_file";
    req["fileType"] = fileType;
    req["fileName"] = info.fileName();
    req["fileData"] = QString(fileBytes.toBase64());

    QTcpSocket socket;
    socket.connectToHost("127.0.0.1", 1234);
    if (!socket.waitForConnected(3000)) {
        errorMsg = "Could not connect to server for upload.";
        return false;
    }

    socket.write(QJsonDocument(req).toJson(QJsonDocument::Compact) + "\n");

    if (!socket.waitForReadyRead(10000)) {
        errorMsg = "Server did not respond to upload.";
        socket.disconnectFromHost();
        return false;
    }

    QByteArray responseData = socket.readAll();
    while (socket.waitForReadyRead(500)) {
        responseData += socket.readAll(); // in case the response arrives in multiple chunks
    }
    socket.disconnectFromHost();

    QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
    if (!responseDoc.isObject()) {
        errorMsg = "Invalid upload response.";
        return false;
    }
    QJsonObject responseObj = responseDoc.object();
    if (responseObj["type"].toString() != "upload_result" || !responseObj["success"].toBool()) {
        errorMsg = responseObj["message"].toString();
        return false;
    }
    outServerPath = responseObj["serverPath"].toString();
    return true;
}

AddEditBookDialog::AddEditBookDialog(const Book &existingBook, QWidget *parent)
    : QDialog(parent), m_book(existingBook)
{
    setupUi(existingBook.id > 0);
}

void AddEditBookDialog::setupUi(bool isEditMode)
{
    setWindowTitle(isEditMode ? "Edit Book" : "Add Book");
    resize(550, 520);
    setStyleSheet(R"(
        QDialog {
            background-color: #120E14;
            color: #EAEAEA;
        }

        QLabel {
            color: #A594B3;
            background-color: #120E14;
            padding: 6px 10px;
            min-width: 90px;
            qproperty-alignment: 'Qt::AlignCenter';
        }

        QLineEdit {
            background-color: #1F1724;
            border: 1px solid #5F2E4F;
            border-radius: 12px;
            padding: 6px 12px;
            color: #EAEAEA;
        }

        QTextEdit {
            background-color: #1F1724;
            border: 1px solid #5F2E4F;
            border-radius: 12px;
            padding: 8px;
            color: #EAEAEA;
        }

        QLineEdit:focus,
        QTextEdit:focus {
            border: 1px solid #7C3E66;
        }

        QDoubleSpinBox {
            background-color: #1F1724;
            border: 1px solid #5F2E4F;
            border-radius: 12px;
            color: #EAEAEA;
            min-height: 32px;
            padding-left: 10px;
            padding-right: 25px;
        }

        QDoubleSpinBox:focus {
            border: 1px solid #7C3E66;
        }

        QPushButton {
            background-color: #7C3E66;
            border: none;
            border-radius: 12px;
            padding: 8px 20px;
            color: white;
            font-weight: bold;
        }

        QPushButton:hover {
            background-color: #5F2E4F;
        }
    )");

    m_titleEdit = new QLineEdit(m_book.title, this);
    m_authorEdit = new QLineEdit(m_book.author, this);
    m_genreEdit = new QLineEdit(m_book.genre, this);

    m_descriptionEdit = new QTextEdit(m_book.description, this);
    m_descriptionEdit->setFixedHeight(100);

    m_priceSpin = new QDoubleSpinBox(this);
    m_priceSpin->setRange(0.0, 999999.0);
    m_priceSpin->setDecimals(2);
    m_priceSpin->setPrefix("$ ");
    m_priceSpin->setValue(m_book.price);
    m_priceSpin->setMinimumHeight(35);

    m_coverPathEdit = new QLineEdit(m_book.coverImagePath, this);
    m_pdfPathEdit = new QLineEdit(m_book.pdfPath, this);

    QPushButton *browseCoverBtn = new QPushButton("Browse...", this);
    QPushButton *browsePdfBtn = new QPushButton("Browse...", this);

    QString browseStyle = R"(
        QPushButton {
            background-color: #1F1724;
            border: 1px solid #5F2E4F;
            border-radius: 12px;
            padding: 6px 15px;
            color: #EAEAEA;
        }
        QPushButton:hover {
            background-color: #7C3E66;
        }
    )";

    browseCoverBtn->setStyleSheet(browseStyle);
    browsePdfBtn->setStyleSheet(browseStyle);

    connect(browseCoverBtn, &QPushButton::clicked, this, [this]() {
        QString localPath = QFileDialog::getOpenFileName(
            this, "Select Cover Image", QString(), "Images (*.png *.jpg *.jpeg *.bmp)");
        if (localPath.isEmpty()) return;

        QString serverPath, errorMsg;
        if (uploadFileToServer(localPath, "cover", serverPath, errorMsg)) {
            m_coverPathEdit->setText(serverPath);
        } else {
            QMessageBox::warning(this, "Upload failed", errorMsg);
        }
    });

    connect(browsePdfBtn, &QPushButton::clicked, this, [this]() {
        QString localPath = QFileDialog::getOpenFileName(
            this, "Select Book PDF", QString(), "PDF Files (*.pdf)");
        if (localPath.isEmpty()) return;

        QString serverPath, errorMsg;
        if (uploadFileToServer(localPath, "pdf", serverPath, errorMsg)) {
            m_pdfPathEdit->setText(serverPath);
        } else {
            QMessageBox::warning(this, "Upload failed", errorMsg);
        }
    });

    QHBoxLayout *coverRow = new QHBoxLayout;
    coverRow->addWidget(m_coverPathEdit);
    coverRow->addWidget(browseCoverBtn);

    QHBoxLayout *pdfRow = new QHBoxLayout;
    pdfRow->addWidget(m_pdfPathEdit);
    pdfRow->addWidget(browsePdfBtn);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    formLayout->setLabelAlignment(Qt::AlignCenter);

    formLayout->setHorizontalSpacing(15);
    formLayout->setVerticalSpacing(14);

    formLayout->addRow("Title", m_titleEdit);
    formLayout->addRow("Price", m_priceSpin);
    formLayout->addRow("Author", m_authorEdit);
    formLayout->addRow("Genre", m_genreEdit);
    formLayout->addRow("Description", m_descriptionEdit);
    formLayout->addRow("Cover Path", coverRow);
    formLayout->addRow("PDF Path", pdfRow);

    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
            Qt::Horizontal,
            this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);
}

Book AddEditBookDialog::resultBook() const
{
    Book b = m_book;
    b.title = m_titleEdit->text().trimmed();
    b.author = m_authorEdit->text().trimmed();
    b.genre = m_genreEdit->text().trimmed();
    b.description = m_descriptionEdit->toPlainText().trimmed();
    b.price = m_priceSpin->value();
    b.coverImagePath = m_coverPathEdit->text().trimmed();
    b.pdfPath = m_pdfPathEdit->text().trimmed();
    return b;
}

///////////////////////////////////////////////////////////////

BookTab::BookTab(QTcpSocket *socket, QWidget *parent)
    : QWidget(parent), m_socket(socket)
{
    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(setupUi());
}

void BookTab::setRowDimmed(QTableWidget *table, int row, bool isDimmed)
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

QWidget* BookTab::setupUi()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(12);

    m_bookSearchEdit = new QLineEdit(page);
    m_bookSearchEdit->setPlaceholderText("🔍 Search books by title or author...");
    m_bookSearchEdit->setStyleSheet(
        "QLineEdit { background-color: #120E14; border: 1px solid #1F1724; border-radius: 6px; "
        "padding: 8px; color: #EAEAEA; font-size: 13px; }"
        "QLineEdit:focus { border: 1px solid #7C3E66; }"
        );
    layout->addWidget(m_bookSearchEdit);

    m_booksTable = new QTableWidget(page);
    m_booksTable->setColumnCount(4);
    m_booksTable->setHorizontalHeaderLabels({"ID", "Title", "Author", "Status"});
    m_booksTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_booksTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_booksTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_booksTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_booksTable->verticalHeader()->setVisible(false);

    m_booksTable->setStyleSheet(
        "QTableWidget { background-color: #09070C; border: 1px solid #1F1724; gridline-color: #1F1724; color: #EAEAEA; }"
        "QTableWidget::item:selected { background-color: #7C3E66; color: white; }"
        "QHeaderView::section { background-color: #120E14; color: #A594B3; font-weight: bold; border: 1px solid #1F1724; padding: 6px; }"
        );
    layout->addWidget(m_booksTable);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_btnApprove = new QPushButton("✨ Approve Book", page);
    m_btnReject = new QPushButton("❌ Reject", page);
    m_btnDeleteBook = new QPushButton("🗑️ Delete Book", page);
    m_btnBookDetails = new QPushButton("👁 View Details", page);
    m_btnEditBook = new QPushButton("✏️ Edit Book", page);

    m_btnApprove->setCursor(Qt::PointingHandCursor);
    m_btnReject->setCursor(Qt::PointingHandCursor);
    m_btnDeleteBook->setCursor(Qt::PointingHandCursor);
    m_btnBookDetails->setCursor(Qt::PointingHandCursor);
    m_btnEditBook->setCursor(Qt::PointingHandCursor);

    m_btnApprove->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #2ECC71; border-radius: 6px; padding: 8px; font-weight: bold; color: #ABEBC6; }"
        "QPushButton:hover { background-color: rgba(46, 204, 113, 50); color: white; }"
        );
    m_btnReject->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #E74C3C; border-radius: 6px; padding: 8px; font-weight: bold; color: #F5B7B1; }"
        "QPushButton:hover { background-color: rgba(231, 76, 60, 50); color: white; }"
        );
    m_btnDeleteBook->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #C0392B; border-radius: 6px; padding: 8px; font-weight: bold; color: #E6B0AA; }"
        "QPushButton:hover { background-color: rgba(192, 57, 43, 50); color: white; }"
        );
    m_btnBookDetails->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #3498DB; border-radius: 6px; padding: 8px; font-weight: bold; color: #AED6F1; }"
        "QPushButton:hover { background-color: rgba(52, 152, 219, 50); color: white; }"
        );
    m_btnEditBook->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #E9A23B; border-radius: 6px; padding: 8px; font-weight: bold; color: #F5D7A0; }"
        "QPushButton:hover { background-color: rgba(233, 162, 59, 50); color: white; }"
        );

    btnLayout->addWidget(m_btnBookDetails);
    btnLayout->addWidget(m_btnEditBook);
    btnLayout->addWidget(m_btnDeleteBook);
    btnLayout->addWidget(m_btnReject);
    btnLayout->addWidget(m_btnApprove);
    layout->addLayout(btnLayout);

    connect(m_bookSearchEdit, &QLineEdit::textChanged, this, &BookTab::filterBooks);
    connect(m_btnApprove, &QPushButton::clicked, this, &BookTab::handleApproveBook);
    connect(m_btnReject, &QPushButton::clicked, this, &BookTab::handleRejectBook);
    connect(m_btnDeleteBook, &QPushButton::clicked, this, &BookTab::handleDeleteBook);
    connect(m_btnBookDetails, &QPushButton::clicked, this, &BookTab::handleViewBookDetails);
    connect(m_btnEditBook, &QPushButton::clicked, this, &BookTab::handleEditBook);

    return page;
}

void BookTab::refreshTable()
{
    QJsonObject request;
    request["action"] = "get_books_list";
    m_socket->write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");
    m_socket->flush();
}

void BookTab::clearTableSelection()
{
    m_booksTable->clearSelection();
}

void BookTab::filterBooks(const QString &text)
{
    for (int i = 0; i < m_booksTable->rowCount(); ++i) {
        bool match = false;
        QString title = m_booksTable->item(i, 1)->text();
        QString author = m_booksTable->item(i, 2)->text();

        if (title.contains(text, Qt::CaseInsensitive) || author.contains(text, Qt::CaseInsensitive)) {
            match = true;
        }
        m_booksTable->setRowHidden(i, !match);
    }
}

void BookTab::handleApproveBook()
{
    int currentRow = m_booksTable->currentRow();
    if (currentRow < 0) return;

    int bookId = m_booksTable->item(currentRow, 0)->text().toInt();

    QJsonObject packet;
    packet["action"] = "set_book_active_status";
    packet["bookId"] = bookId;
    packet["active_status"] = true;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void BookTab::handleRejectBook()
{
    int currentRow = m_booksTable->currentRow();
    if (currentRow < 0) return;
    int bookId = m_booksTable->item(currentRow, 0)->text().toInt();

    QJsonObject packet;
    packet["action"] = "set_book_active_status";
    packet["bookId"] = bookId;
    packet["active_status"] = false;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void BookTab::handleDeleteBook()
{
    int currentRow = m_booksTable->currentRow();
    if (currentRow < 0) return;
    int bookId = m_booksTable->item(currentRow, 0)->text().toInt();

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Delete",
                                                              "Are you sure you want to completely delete this book from the database?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QJsonObject packet;
        packet["action"] = "books_delete";
        packet["bookId"] = bookId;
        m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
    }
}

void BookTab::handleViewBookDetails()
{
    int currentRow = m_booksTable->currentRow();
    if (currentRow < 0) return;
    int bookId = m_booksTable->item(currentRow, 0)->text().toInt();

    m_pendingBookDetailPurpose = "view";
    QJsonObject packet;
    packet["action"] = "get_book_details";
    packet["bookId"] = bookId;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void BookTab::handleEditBook()
{
    int currentRow = m_booksTable->currentRow();
    if (currentRow < 0) return;
    int bookId = m_booksTable->item(currentRow, 0)->text().toInt();

    m_pendingBookDetailPurpose = "edit";
    QJsonObject packet;
    packet["action"] = "get_book_details";
    packet["bookId"] = bookId;
    m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
}

void BookTab::handleServerResponse(const QJsonObject &response)
{
    QString action = response["action"].toString();

    if (response.contains("type") && response["type"].toString() == "table_refresh_required") {
        if (response["target_table"].toString() == "book") {
            refreshTable();
        }
        return;
    }

    if (action == "books_list_response" && response["status"] == "success") {
        QJsonArray books = response["data"].toArray();
        m_booksTable->setRowCount(0);

        for (int i = 0; i < books.size(); ++i) {
            QJsonObject b = books[i].toObject();
            m_booksTable->insertRow(i);

            m_booksTable->setItem(i, 0, new QTableWidgetItem(QString::number(b["id"].toInt())));
            m_booksTable->setItem(i, 1, new QTableWidgetItem(b["title"].toString()));
            m_booksTable->setItem(i, 2, new QTableWidgetItem(b["author"].toString()));

            bool isActive = b["isActive"].toBool();
            m_booksTable->setItem(i, 3, new QTableWidgetItem(isActive ? "Approved" : "Pending/Rejected"));

            if (!isActive) setRowDimmed(m_booksTable, i, true);
        }
    }
    else if (action == "delete_book_response" && response["status"] == "success") {
        int currentRow = m_booksTable->currentRow();
        if (currentRow >= 0) {
            m_booksTable->removeRow(currentRow);
        }
        QMessageBox::information(this, "Success", "Book completely deleted from the database.");
    }
    else if (action == "book_details_response" && response["status"].toString() == "success") {
        QJsonObject data = response["data"].toObject();
        if (m_pendingBookDetailPurpose == "edit") {
            Book b;
            b.id = data["id"].toInt();
            b.title = data["title"].toString();
            b.author = data["author"].toString();
            b.genre = data["genre"].toString();
            b.description = data["description"].toString();
            b.price = data["price"].toDouble();
            b.coverImagePath = data["coverImagePath"].toString();
            b.pdfPath = data["pdfPath"].toString();
            b.averageRating = data["averageRating"].toDouble();
            b.status = data["isActive"].toBool();

            AddEditBookDialog dialog(b, this);
            if (dialog.exec() == QDialog::Accepted) {
                Book updated = dialog.resultBook();
                QJsonObject packet;
                packet["action"] = "book_update";
                packet["id"] = updated.id;
                packet["title"] = updated.title;
                packet["author"] = updated.author;
                packet["genre"] = updated.genre;
                packet["description"] = updated.description;
                packet["price"] = updated.price;
                packet["coverImagePath"] = updated.coverImagePath;
                packet["pdfPath"] = updated.pdfPath;
                m_socket->write(QJsonDocument(packet).toJson(QJsonDocument::Compact) + "\n");
            }
        } else {
            showBookDetailsDialog(response["data"].toObject());
        }
    }
    else if (action == "book_update_response") {
        if (response["status"].toString() == "success") {
            QMessageBox::information(this, "Success", "Book information updated successfully.");
            refreshTable();
        } else {
            QMessageBox::warning(this, "Update Failed", response["message"].toString());
        }
    }
}

void BookTab::showBookDetailsDialog(const QJsonObject &data)
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Book Details");
    dialog->resize(560, 520);
    dialog->setStyleSheet("background-color: #09070C; color: #EAEAEA; font-family: 'Segoe UI', Arial;");

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    mainLayout->setContentsMargins(24, 24, 24, 20);
    mainLayout->setSpacing(14);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(16);

    QLabel *cover = new QLabel("📖", dialog);
    cover->setFixedSize(64, 64);
    cover->setAlignment(Qt::AlignCenter);
    cover->setStyleSheet("font-size: 30px; background-color: #1F1724; border-radius: 32px;");

    QVBoxLayout *headerTextLayout = new QVBoxLayout();
    headerTextLayout->setSpacing(2);

    QLabel *titleLabel = new QLabel(data["title"].toString(), dialog);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #FFFFFF;");

    QLabel *authorLabel = new QLabel("by " + data["author"].toString(), dialog);
    authorLabel->setStyleSheet("font-size: 12px; color: #9A8FA0;");

    bool isActive = data["isActive"].toBool();
    QLabel *statusBadge = new QLabel(isActive ? "✅ APPROVED" : "🚫 PENDING/REJECTED", dialog);
    statusBadge->setStyleSheet(
        QString("background-color: %1; color: white; font-size: 10px; font-weight: bold; "
                "padding: 3px 10px; border-radius: 8px; letter-spacing: 1px;")
            .arg(isActive ? "#268730" : "#8d1f1f"));
    statusBadge->setFixedHeight(20);
    statusBadge->setMaximumWidth(220);

    headerTextLayout->addWidget(titleLabel);
    headerTextLayout->addWidget(authorLabel);
    headerTextLayout->addSpacing(4);
    headerTextLayout->addWidget(statusBadge);

    headerLayout->addWidget(cover);
    headerLayout->addLayout(headerTextLayout, 1);
    mainLayout->addLayout(headerLayout);

    QString cardStyle =
        "QGroupBox { border: 1px solid #1F1724; border-radius: 10px; margin-top: 8px; padding-top: 16px; "
        "color: #A594B3; font-weight: bold; background-color: #0F0C12; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }";

    QGroupBox *infoBox = new QGroupBox("Book Information", dialog);
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
        QLabel *val = new QLabel(value.isEmpty() ? "—" : value);
        val->setStyleSheet(valueStyle);
        val->setWordWrap(true);
        formLayout->addRow(lbl, val);
    };

    addRow("Genre:", data["genre"].toString());
    addRow("Price:", "$" + QString::number(data["price"].toDouble(), 'f', 2));
    double rating = data["averageRating"].toDouble();
    addRow("Avg Rating:", rating > 0.0 ? QString::number(rating, 'f', 1) + " ⭐" : "N/A");
    addRow("Description:", data["description"].toString());

    mainLayout->addWidget(infoBox);
    mainLayout->addStretch();

    QPushButton *closeDetailsBtn = new QPushButton("Close", dialog);
    closeDetailsBtn->setCursor(Qt::PointingHandCursor);
    closeDetailsBtn->setStyleSheet(
        "QPushButton { background-color: #1F1724; border: none; border-radius: 6px; padding: 10px; color: white; font-weight: bold; }"
        "QPushButton:hover { background-color: #7C3E66; }"
        );
    connect(closeDetailsBtn, &QPushButton::clicked, dialog , &QDialog::accept);

    QHBoxLayout *detailsFooterLayout = new QHBoxLayout();
    detailsFooterLayout->addStretch();
    detailsFooterLayout->addWidget(closeDetailsBtn);
    mainLayout->addLayout(detailsFooterLayout);
}

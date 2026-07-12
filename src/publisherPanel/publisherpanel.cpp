#include "publisherpanel.h"
#include "bookcardwidget.h"
#include "addeditbookdialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>

PublisherPanel::PublisherPanel(int publisherId, QWidget *parent)
    : QWidget(parent), m_publisherId(publisherId)
{
    setupUi();

    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::readyRead, this, &PublisherPanel::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &PublisherPanel::onSocketError);
    connect(m_socket, &QTcpSocket::connected, this, [this]() {
        requestBooks();
        requestStats();
    });
    m_socket->connectToHost("127.0.0.1", 1234);

    switchPage(1); // default to "My Books"
}

void PublisherPanel::sendRequest(const QJsonObject &requestObj)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, "Not connected", "Not connected to the server.");
        return;
    }
    m_socket->write(QJsonDocument(requestObj).toJson(QJsonDocument::Compact) + "\n");
}

void PublisherPanel::requestBooks()
{
    qDebug() << "Requesting books";
    QJsonObject req;
    req["action"] = "publisher_get_books";
    req["publisherId"] = m_publisherId;
    sendRequest(req);
}

void PublisherPanel::requestStats()
{
    QJsonObject req;
    req["action"] = "publisher_get_stats";
    req["publisherId"] = m_publisherId;
    sendRequest(req);
}

void PublisherPanel::setupUi()
{
    setStyleSheet("background-color: #060508; color: #EAEAEA; font-family: 'Segoe UI', Arial;");

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget *sidebar = new QWidget(this);
    sidebar->setFixedWidth(220);
    sidebar->setStyleSheet("background-color: #120E14; border-left: 1px solid #1F1724;");

    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(15, 25, 15, 25);
    sidebarLayout->setSpacing(12);

    QLabel *avatarLabel = new QLabel("📖", sidebar);
    avatarLabel->setStyleSheet("font-size: 40px; border: none; background: transparent;");
    avatarLabel->setAlignment(Qt::AlignCenter);

    QLabel *roleLabel = new QLabel("Publisher", sidebar);
    roleLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #FFEAD2; border: none; background: transparent;");
    roleLabel->setAlignment(Qt::AlignCenter);

    sidebarLayout->addWidget(avatarLabel);
    sidebarLayout->addWidget(roleLabel);
    sidebarLayout->addSpacing(15);

    m_btnStats = new QPushButton("📊 Book Statistics", sidebar);
    m_btnBooks = new QPushButton("📚 My Books", sidebar);
    m_btnLogout = new QPushButton("🚪 Logout", sidebar);

    QString menuBtnStyle =
        "QPushButton { background-color: transparent; border: none; border-radius: 8px; padding: 10px; font-size: 13px; color: #9A8FA0; text-align: left; padding-left: 12px; }"
        "QPushButton:hover { background-color: #1F1724; color: #EAEAEA; }";

    m_btnStats->setStyleSheet(menuBtnStyle);
    m_btnBooks->setStyleSheet(menuBtnStyle);
    m_btnStats->setCursor(Qt::PointingHandCursor);
    m_btnBooks->setCursor(Qt::PointingHandCursor);

    m_btnLogout->setCursor(Qt::PointingHandCursor);
    m_btnLogout->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #7C3E66; border-radius: 8px; padding: 8px; font-weight: bold; color: #D9C2D1; text-align: left; padding-left: 12px; }"
        "QPushButton:hover { background-color: rgba(124, 62, 102, 60); color: white; border: 1px solid #B06B96; }"
        );

    sidebarLayout->addWidget(m_btnStats);
    sidebarLayout->addWidget(m_btnBooks);
    sidebarLayout->addStretch();
    sidebarLayout->addWidget(m_btnLogout);

    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->addWidget(createStatsPage()); // index 0
    m_stackedWidget->addWidget(createBooksPage()); // index 1

    mainLayout->addWidget(m_stackedWidget);
    mainLayout->addWidget(sidebar);

    connect(m_btnStats, &QPushButton::clicked, this, [this]() { switchPage(0); requestStats(); });
    connect(m_btnBooks, &QPushButton::clicked, this, [this]() { switchPage(1); });
    connect(m_btnLogout, &QPushButton::clicked, this, &PublisherPanel::logoutRequested);
}

QWidget* PublisherPanel::createStatsPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(16);

    auto makeStatCard = [page](const QString &title, QLabel *&valueLabelOut) -> QWidget* {
        QWidget *card = new QWidget(page);
        card->setStyleSheet("background-color: #120E14; border: 1px solid #1F1724; border-radius: 10px;");
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        QLabel *titleLabel = new QLabel(title, card);
        titleLabel->setStyleSheet("color: #A594B3; font-size: 12px;");
        valueLabelOut = new QLabel("—", card);
        valueLabelOut->setStyleSheet("color: #EAEAEA; font-size: 24px; font-weight: bold;");
        cardLayout->addWidget(titleLabel);
        cardLayout->addWidget(valueLabelOut);
        return card;
    };

    QGridLayout *statsGrid = new QGridLayout();
    statsGrid->setSpacing(16);
    statsGrid->addWidget(makeStatCard("Total Books", m_statBookCount), 0, 0);
    statsGrid->addWidget(makeStatCard("Total Sales", m_statTotalSales), 0, 1);
    statsGrid->addWidget(makeStatCard("Average Rating", m_statAvgRating), 1, 0);
    statsGrid->addWidget(makeStatCard("Total Income", m_statTotalIncome), 1, 1);

    layout->addLayout(statsGrid);
    layout->addStretch();

    return page;
}

QWidget* PublisherPanel::createBooksPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    m_bookSearchEdit = new QLineEdit(page);
    m_bookSearchEdit->setPlaceholderText("🔍 Search your books by title...");
    m_bookSearchEdit->setStyleSheet(
        "QLineEdit { background-color: #120E14; border: 1px solid #1F1724; border-radius: 6px; "
        "padding: 8px; color: #EAEAEA; font-size: 13px; }"
        "QLineEdit:focus { border: 1px solid #7C3E66; }"
        );
    layout->addWidget(m_bookSearchEdit);

    m_booksScrollArea = new QScrollArea(page);
    m_booksScrollArea->setWidgetResizable(true);
    m_booksScrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");

    m_booksGridContainer = new QWidget(m_booksScrollArea);
    m_booksGridContainer->setStyleSheet("background: transparent;");

    m_booksGrid = new QGridLayout(m_booksGridContainer);
    m_booksGrid->setSpacing(16);
    m_booksGrid->setContentsMargins(0, 0, 0, 0);

    m_booksScrollArea->setWidget(m_booksGridContainer);
    layout->addWidget(m_booksScrollArea);

    connect(m_bookSearchEdit, &QLineEdit::textChanged, this, &PublisherPanel::filterBooks);

    populateBooksGrid(QVector<Book>());

    return page;
}

void PublisherPanel::populateBooksGrid(const QVector<Book> &books)
{
    m_currentBooks = books;

    QLayoutItem *item;
    while ((item = m_booksGrid->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    const int columns = 5;
    int row = 0, col = 0;

    for (const Book &book : std::as_const(books)) {
        if (book.status == -1) {
            continue;
        }
        BookCardWidget *card = new BookCardWidget(book, m_booksGridContainer);
        connect(card, &BookCardWidget::editRequested, this, &PublisherPanel::handleEditBook);
        connect(card, &BookCardWidget::deleteRequested, this, &PublisherPanel::handleDeleteBook);
        connect(card, &BookCardWidget::offerRequested, this, &PublisherPanel::handleSetOffer);
        connect(card, &BookCardWidget::toggleActiveRequested, this, &PublisherPanel::handleToggleActive);
        m_booksGrid->addWidget(card, row, col);
        col++;
        if (col >= columns) { col = 0; row++; }
    }

    QPushButton *addCard = new QPushButton("+", m_booksGridContainer);
    addCard->setFixedSize(180, 240);
    addCard->setCursor(Qt::PointingHandCursor);
    addCard->setStyleSheet(
        "QPushButton { background-color: transparent; border: 2px dashed #5F2E4F; border-radius: 8px; font-size: 32px; color: #7C3E66; }"
        "QPushButton:hover { background-color: rgba(124, 62, 102, 30); color: #B06B96; border: 2px dashed #7C3E66; }"
        );
    connect(addCard, &QPushButton::clicked, this, &PublisherPanel::handleAddBook);

    m_booksGrid->addWidget(addCard, row, col);
}

void PublisherPanel::handleAddBook()
{
    Book emptyBook;
    emptyBook.publisherId = m_publisherId;

    AddEditBookDialog dialog(emptyBook, this);
    if (dialog.exec() == QDialog::Accepted) {
        Book b = dialog.resultBook();
        QJsonObject req;
        req["action"] = "publisher_add_book";
        req["publisherId"] = m_publisherId;
        req["title"] = b.title;
        req["author"] = b.author;
        req["genre"] = b.genre;
        req["description"] = b.description;
        req["price"] = b.price;
        req["coverImagePath"] = b.coverImagePath;
        req["pdfPath"] = b.pdfPath;
        sendRequest(req);
    }
}

void PublisherPanel::handleEditBook(int bookId)
{
    auto it = std::find_if(m_currentBooks.begin(), m_currentBooks.end(),
                           [bookId](const Book &b) { return b.id == bookId; });
    if (it == m_currentBooks.end()) return;

    AddEditBookDialog dialog(*it, this);
    if (dialog.exec() == QDialog::Accepted) {
        Book b = dialog.resultBook();
        QJsonObject req;
        req["action"] = "publisher_update_book";
        req["id"] = b.id;
        req["publisherId"] = m_publisherId;
        req["title"] = b.title;
        req["author"] = b.author;
        req["genre"] = b.genre;
        req["description"] = b.description;
        req["price"] = b.price;
        req["coverImagePath"] = b.coverImagePath;
        req["pdfPath"] = b.pdfPath;
        sendRequest(req);
    }
}

void PublisherPanel::handleDeleteBook(int bookId)
{
    auto reply = QMessageBox::question(this, "Delete Book",
                                       "Are you sure you want to remove this book? It will be hidden from readers.",
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    QJsonObject req;
    req["action"] = "publisher_delete_book";
    req["bookId"] = bookId;
    sendRequest(req);
}

void PublisherPanel::handleToggleActive(int bookId, int newStatus)
{
    QJsonObject req;
    req["action"] = "publisher_set_book_status";
    req["bookId"] = bookId;
    req["publisherId"] = m_publisherId;
    req["status"] = newStatus;
    sendRequest(req);
}

void PublisherPanel::handleSetOffer(int bookId)
{
    // TODO: open SetOfferDialog, then send "publisher_add_discount" with the result
    Q_UNUSED(bookId);
}

void PublisherPanel::onReadyRead()
{
    while (m_socket->canReadLine()) {

        QByteArray data = m_socket->readLine().trimmed();

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);

        if (err.error != QJsonParseError::NoError) {
            qWarning() << "JSON parse error:" << err.errorString();
            continue;
        }

        if (!doc.isObject()) {
            continue;
        }
    const QJsonObject responseObj = doc.object();
    const QString type = responseObj["type"].toString();

    if (type == "publisher_books_list") {
        QVector<Book> books;
        const QJsonArray booksArray = responseObj["books"].toArray();
        for (const QJsonValue &val : booksArray) {
            const QJsonObject bo = val.toObject();
            Book b;
            b.id = bo["id"].toInt();
            b.publisherId = m_publisherId;
            b.title = bo["title"].toString();
            b.author = bo["author"].toString();
            b.genre = bo["genre"].toString();
            b.description = bo["description"].toString();
            b.price = bo["price"].toDouble();
            b.coverImagePath = bo["coverImagePath"].toString();
            b.pdfPath = bo["pdfPath"].toString();
            b.status = bo["status"].toInt();
            b.averageRating = bo["averageRating"].toDouble();
            b.totalSales = bo["totalSales"].toInt();
            books.push_back(b);
        }
        m_allBooks = books;
        filterBooks(m_bookSearchEdit->text());
    }
    else if (type == "publisher_stats") {
        m_statBookCount->setText(QString::number(responseObj["bookCount"].toInt()));
        m_statTotalSales->setText(QString::number(responseObj["totalSales"].toInt()));
        m_statAvgRating->setText(QString::number(responseObj["averageRating"].toDouble(), 'f', 1));
        m_statTotalIncome->setText("$" + QString::number(responseObj["totalIncome"].toDouble(), 'f', 2));
    }
    else if (type == "action_result") {
        const bool success = responseObj["success"].toBool();
        if (success) {
            requestBooks(); // refresh grid after add/edit/delete/status change
            requestStats();
        } else {
            QMessageBox::warning(this, "Action failed", responseObj["message"].toString());
        }
    }
    else {
        const QString status = responseObj["status"].toString();
        if (status == "success") {
            requestBooks();
            requestStats();
        } else if (status == "error") {
            QMessageBox::warning(this, "Action failed", responseObj["message"].toString());
        }
    }
}
}

void PublisherPanel::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    qWarning() << "PublisherPanel socket error:" << m_socket->errorString();
}

void PublisherPanel::switchPage(int index)
{
    m_stackedWidget->setCurrentIndex(index);
    updateButtonStyles(index);
}

void PublisherPanel::updateButtonStyles(int currentIndex)
{
    QString normalStyle =
        "QPushButton { background-color: transparent; border: none; border-radius: 8px; padding: 10px; font-size: 13px; color: #9A8FA0; text-align: left; padding-left: 12px; }"
        "QPushButton:hover { background-color: #1F1724; color: #EAEAEA; }";

    QString activeStyle =
        "QPushButton { background-color: #7C3E66; border: none; border-radius: 8px; padding: 10px; font-size: 13px; font-weight: bold; color: #FFFFFF; text-align: left; padding-left: 12px; }";

    m_btnStats->setStyleSheet(currentIndex == 0 ? activeStyle : normalStyle);
    m_btnBooks->setStyleSheet(currentIndex == 1 ? activeStyle : normalStyle);
}

void PublisherPanel::filterBooks(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        populateBooksGrid(m_allBooks);
        return;
    }

    QVector<Book> filtered;
    for (const Book &b : std::as_const(m_allBooks)) {
        if (b.title.contains(text, Qt::CaseInsensitive)) {
            filtered.push_back(b);
        }
    }
    populateBooksGrid(filtered);
}
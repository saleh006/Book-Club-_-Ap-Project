#include "publisherpanel.h"
#include "bookcardwidget.h"
#include "addeditbookdialog.h"
#include "setofferdialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QValueAxis>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QHeaderView>
#include <QScrollArea>
#include <QGridLayout>

static const char *kCardBg     = "#120E14";
static const char *kCardBorder = "#1F1724";
static const char *kAccent     = "#A85CF0";
static const char *kTextDim    = "#9A8FA0";
static const char *kGrid       = "#2A2233";

PublisherPanel::PublisherPanel(int publisherId, const QString &fullName, const QString &username, QWidget *parent)
    : QWidget(parent), m_publisherId(publisherId), m_fullName(fullName), m_username(username)
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

    m_nameLabel = new QLabel(m_fullName.isEmpty() ? m_username : m_fullName, sidebar);
    m_nameLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #EAEAEA; border: none; background: transparent;");
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setWordWrap(true);

    m_usernameLabel = new QLabel("@" + m_username, sidebar);
    m_usernameLabel->setStyleSheet("font-size: 11px; color: #9A8FA0; border: none; background: transparent;");
    m_usernameLabel->setAlignment(Qt::AlignCenter);

    sidebarLayout->addWidget(avatarLabel);
    sidebarLayout->addWidget(roleLabel);
    sidebarLayout->addSpacing(8);
    sidebarLayout->addWidget(m_nameLabel);
    sidebarLayout->addWidget(m_usernameLabel);
    sidebarLayout->addSpacing(15);
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

// QWidget* PublisherPanel::createStatsPage()
// {
//     QWidget *page = new QWidget(this);
//     QVBoxLayout *layout = new QVBoxLayout(page);
//     layout->setContentsMargins(30, 30, 30, 30);
//     layout->setSpacing(16);

//     auto makeStatCard = [page](const QString &title, QLabel *&valueLabelOut) -> QWidget* {
//         QWidget *card = new QWidget(page);
//         card->setStyleSheet("background-color: #120E14; border: 1px solid #1F1724; border-radius: 10px;");
//         QVBoxLayout *cardLayout = new QVBoxLayout(card);
//         QLabel *titleLabel = new QLabel(title, card);
//         titleLabel->setStyleSheet("color: #A594B3; font-size: 12px;");
//         valueLabelOut = new QLabel("—", card);
//         valueLabelOut->setStyleSheet("color: #EAEAEA; font-size: 24px; font-weight: bold;");
//         cardLayout->addWidget(titleLabel);
//         cardLayout->addWidget(valueLabelOut);
//         return card;
//     };

//     QGridLayout *statsGrid = new QGridLayout();
//     statsGrid->setSpacing(16);
//     statsGrid->addWidget(makeStatCard("Total Books", m_statBookCount), 0, 0);
//     statsGrid->addWidget(makeStatCard("Total Sales", m_statTotalSales), 0, 1);
//     statsGrid->addWidget(makeStatCard("Average Rating", m_statAvgRating), 1, 0);
//     statsGrid->addWidget(makeStatCard("Total Income", m_statTotalIncome), 1, 1);

//     layout->addLayout(statsGrid);
//     layout->addStretch();

//     return page;
// }

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
    SetOfferDialog dialog(bookId, this);
    if (dialog.exec() == QDialog::Accepted) {
        Discount d = dialog.resultDiscount();

        if (d.startDate >= d.endDate) {
            QMessageBox::warning(this, "Invalid dates", "Start time must be before end time.");
            return;
        }

        QJsonObject req;
        req["action"] = "publisher_add_discount";
        req["publisherId"] = m_publisherId;
        req["bookId"] = d.bookId;
        req["type"] = d.type;
        req["value"] = d.value;
        req["startDate"] = d.startDate.toUTC().toString(Qt::ISODate);
        req["endDate"] = d.endDate.toUTC().toString(Qt::ISODate);
        sendRequest(req);
    }
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
        m_allBooks = books;
        filterBooks(m_bookSearchEdit->text());
        updateDashboard();
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
    else if (type == "publisher_sales_trend") {
        updateSalesTrend(responseObj["points"].toArray());
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

static void styleAxis(QAbstractAxis *ax)
{
    ax->setLabelsBrush(QBrush(QColor(kTextDim)));
    ax->setGridLineColor(QColor(kGrid));
    ax->setLinePenColor(QColor(kGrid));
    QFont f = ax->labelsFont(); f.setPointSize(8); ax->setLabelsFont(f);
}

static QChart *makeDarkChart()
{
    auto *chart = new QChart;
    chart->setBackgroundBrush(Qt::transparent);
    chart->legend()->hide();
    chart->setMargins(QMargins(4, 4, 4, 4));
    return chart;
}

void PublisherPanel::replaceChart(QChartView *view, QChart *chart)
{
    QChart *old = view->chart();
    view->setChart(chart);
    delete old;
}

QWidget *PublisherPanel::makeStatCard(const QString &icon, const QString &iconBg,
                                      const QString &title, const QString &subtitle,
                                      QLabel *&valueOut)
{
    auto *card = new QWidget(this);
    card->setStyleSheet(QString("background-color:%1;border:1px solid %2;border-radius:10px;")
                            .arg(kCardBg, kCardBorder));
    auto *h = new QHBoxLayout(card);
    h->setContentsMargins(16, 14, 16, 14);
    h->setSpacing(14);

    auto *iconLabel = new QLabel(icon, card);
    iconLabel->setFixedSize(48, 48);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet(QString("background-color:%1;border:none;border-radius:10px;font-size:22px;")
                                 .arg(iconBg));

    auto *v = new QVBoxLayout;
    v->setSpacing(2);
    auto *titleLabel = new QLabel(title, card);
    titleLabel->setStyleSheet("color:#EAEAEA;font-size:12px;font-weight:bold;border:none;background:transparent;");
    valueOut = new QLabel("—", card);
    valueOut->setStyleSheet("color:#FFFFFF;font-size:24px;font-weight:bold;border:none;background:transparent;");
    auto *subLabel = new QLabel(subtitle, card);
    subLabel->setStyleSheet(QString("color:%1;font-size:10px;border:none;background:transparent;").arg(kTextDim));
    v->addWidget(titleLabel);
    v->addWidget(valueOut);
    v->addWidget(subLabel);

    h->addWidget(iconLabel);
    h->addLayout(v);
    h->addStretch();
    return card;
}

QWidget *PublisherPanel::makeSectionCard(const QString &icon, const QString &title,
                                         QWidget *content, QWidget *headerRight)
{
    auto *card = new QWidget(this);
    card->setStyleSheet(QString("QWidget{background-color:%1;border:1px solid %2;border-radius:10px;}")
                            .arg(kCardBg, kCardBorder));
    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(16, 14, 16, 14);
    v->setSpacing(10);

    auto *header = new QHBoxLayout;
    auto *titleLabel = new QLabel(icon + "  " + title, card);
    titleLabel->setStyleSheet("color:#EAEAEA;font-size:13px;font-weight:bold;border:none;background:transparent;");
    header->addWidget(titleLabel);
    header->addStretch();
    if (headerRight) header->addWidget(headerRight);
    v->addLayout(header);
    v->addWidget(content);
    return card;
}

QTableWidget *PublisherPanel::makeTopTable()
{
    auto *t = new QTableWidget(5, 3, this);
    t->setHorizontalHeaderLabels({"#", "Book Title", "Sales"});
    t->verticalHeader()->setVisible(false);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionMode(QAbstractItemView::NoSelection);
    t->setFocusPolicy(Qt::NoFocus);
    t->setShowGrid(false);
    t->setFixedHeight(232);
    t->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    t->setColumnWidth(0, 40);
    t->setColumnWidth(2, 70);
    t->setStyleSheet(QString(
                         "QTableWidget{background:transparent;border:none;color:#EAEAEA;font-size:12px;}"
                         "QTableWidget::item{border-bottom:1px solid %1;padding:4px;}"
                         "QHeaderView::section{background-color:#1A141F;color:%2;border:none;padding:6px;font-size:11px;}")
                         .arg(kCardBorder, kTextDim));
    return t;
}

QWidget *PublisherPanel::createStatsPage()
{
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea{border:none;background:transparent;}");

    auto *page = new QWidget;
    page->setStyleSheet("background:transparent;");
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 25, 30, 25);
    layout->setSpacing(18);

    // header
    m_welcomeLabel = new QLabel(QString("Welcome back, %1!")
                                    .arg(m_fullName.isEmpty() ? m_username : m_fullName), page);
    m_welcomeLabel->setStyleSheet("color:#FFFFFF;font-size:22px;font-weight:bold;border:none;");
    auto *sub = new QLabel("Here's an overview of your books and sales performance.", page);
    sub->setStyleSheet(QString("color:%1;font-size:12px;border:none;").arg(kTextDim));
    layout->addWidget(m_welcomeLabel);
    layout->addWidget(sub);

    // stat cards
    auto *cardsRow = new QHBoxLayout;
    cardsRow->setSpacing(16);
    cardsRow->addWidget(makeStatCard("📚", "#8B5CF6", "Total Books",    "Published books", m_statBookCount));
    cardsRow->addWidget(makeStatCard("🛒", "#EC4899", "Total Sales",    "Books sold",      m_statTotalSales));
    cardsRow->addWidget(makeStatCard("💲", "#22C55E", "Total Revenue",  "Total income",    m_statTotalIncome));
    cardsRow->addWidget(makeStatCard("⭐", "#EAB308", "Average Rating", "Out of 5",        m_statAvgRating));
    layout->addLayout(cardsRow);

    // chart views
    auto makeView = [this]() {
        auto *v = new QChartView(makeDarkChart(), this);
        v->setRenderHint(QPainter::Antialiasing);
        v->setStyleSheet("background:transparent;border:none;");
        v->setMinimumHeight(240);
        return v;
    };
    m_trendView  = makeView();
    m_cmpView    = makeView();
    m_ratingView = makeView();
    m_pieView    = makeView();

    m_bestTable  = makeTopTable();
    m_worstTable = makeTopTable();

     //(daily / weekly / monthly per the spec)
    m_trendCombo = new QComboBox(this);
    m_trendCombo->addItems({"Monthly", "Weekly", "Daily"});
    m_trendCombo->setStyleSheet(QString(
                                    "QComboBox{background-color:#1A141F;border:1px solid %1;border-radius:6px;"
                                    "padding:4px 10px;color:#EAEAEA;font-size:11px;}").arg(kCardBorder));
    connect(m_trendCombo, &QComboBox::currentTextChanged, this,
            [this](const QString &) { requestSalesTrend(); });
    auto *pieRow = new QWidget(this);
    pieRow->setStyleSheet("background:transparent;border:none;");
    auto *pieRowLayout = new QHBoxLayout(pieRow);
    pieRowLayout->setContentsMargins(0, 0, 0, 0);
    pieRowLayout->addWidget(m_pieView, 3);
    auto *legendHolder = new QWidget(pieRow);
    legendHolder->setStyleSheet("background:transparent;border:none;");
    m_pieLegendLayout = new QVBoxLayout(legendHolder);
    m_pieLegendLayout->setContentsMargins(0, 20, 0, 20);
    m_pieLegendLayout->setSpacing(8);
    m_pieLegendLayout->addStretch();
    pieRowLayout->addWidget(legendHolder, 2);

    auto *grid = new QGridLayout;
    grid->setSpacing(18);
    grid->addWidget(makeSectionCard("🏆", "Top 5 Best Selling Books", m_bestTable),            0, 0);
    grid->addWidget(makeSectionCard("📈", "Sales Trend", m_trendView, m_trendCombo),           0, 1);
    grid->addWidget(makeSectionCard("📉", "Top 5 Worst Selling Books", m_worstTable),          1, 0);
    grid->addWidget(makeSectionCard("📊", "Sales Comparison (Top 5 Books)", m_cmpView),        1, 1);
    grid->addWidget(makeSectionCard("⭐", "Average Rating per Book", m_ratingView),            2, 0);
    grid->addWidget(makeSectionCard("🥧", "Revenue Share per Book", pieRow),                   2, 1);
    grid->setColumnStretch(0, 2);
    grid->setColumnStretch(1, 3);
    layout->addLayout(grid);
    layout->addStretch();

    scroll->setWidget(page);
    return scroll;
}

// ---------- data → UI ----------

void PublisherPanel::fillTopTable(QTableWidget *table, const QVector<Book> &books)
{
    table->clearContents();
    table->setRowCount(qMin(5, static_cast<int>(books.size())));
    for (int i = 0; i < table->rowCount(); ++i) {
        auto *rank = new QTableWidgetItem(QString::number(i + 1));
        auto *title = new QTableWidgetItem(books[i].title);
        auto *sales = new QTableWidgetItem(QString::number(books[i].totalSales));
        sales->setForeground(QColor(kAccent));
        sales->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->setItem(i, 0, rank);
        table->setItem(i, 1, title);
        table->setItem(i, 2, sales);
    }
}

void PublisherPanel::updateDashboard()
{
    QVector<Book> books;
    for (const Book &b : std::as_const(m_allBooks))
        if (b.status != -1) books.push_back(b);
    if (books.isEmpty()) return;

    // best / worst tables
    QVector<Book> sorted = books;
    std::sort(sorted.begin(), sorted.end(),
              [](const Book &a, const Book &b) { return a.totalSales > b.totalSales; });
    fillTopTable(m_bestTable, sorted);
    QVector<Book> worst = sorted;
    std::reverse(worst.begin(), worst.end());
    fillTopTable(m_worstTable, worst);

    const QVector<Book> top5 = sorted.mid(0, 5);
    auto wrapTitle = [](QString t) { return t.replace(' ', '\n'); };

    {
        auto *set = new QBarSet("Sales");
        set->setColor(QColor(kAccent));
        set->setLabelColor(QColor("#EAEAEA"));
        QStringList cats;
        double maxV = 0;
        for (const Book &b : top5) { *set << b.totalSales; cats << wrapTitle(b.title); maxV = qMax(maxV, double(b.totalSales)); }
        auto *series = new QBarSeries;
        series->append(set);
        series->setLabelsVisible(true);
        series->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd);

        QChart *chart = makeDarkChart();
        chart->addSeries(series);
        auto *ax = new QBarCategoryAxis; ax->append(cats); styleAxis(ax);
        auto *ay = new QValueAxis; ay->setRange(0, maxV * 1.2); ay->setLabelFormat("%d"); styleAxis(ay);
        chart->addAxis(ax, Qt::AlignBottom); chart->addAxis(ay, Qt::AlignLeft);
        series->attachAxis(ax); series->attachAxis(ay);
        replaceChart(m_cmpView, chart);
    }

    {
        auto *set = new QBarSet("Rating");
        set->setColor(QColor(kAccent));
        set->setLabelColor(QColor("#EAEAEA"));
        QStringList cats;
        for (const Book &b : books) { *set << b.averageRating; cats << wrapTitle(b.title); }
        auto *series = new QBarSeries;
        series->append(set);
        series->setLabelsVisible(true);
        series->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd);
        series->setLabelsFormat("@value");

        QChart *chart = makeDarkChart();
        chart->addSeries(series);
        auto *ax = new QBarCategoryAxis; ax->append(cats); styleAxis(ax);
        auto *ay = new QValueAxis; ay->setRange(0, 5.5); ay->setTickCount(6); ay->setLabelFormat("%d"); styleAxis(ay);
        chart->addAxis(ax, Qt::AlignBottom); chart->addAxis(ay, Qt::AlignLeft);
        series->attachAxis(ax); series->attachAxis(ay);
        replaceChart(m_ratingView, chart);
    }

    {
        static const QStringList palette = {"#A85CF0", "#EC4899", "#22C55E", "#EAB308", "#3B82F6"};
        double total = 0;
        QVector<QPair<Book, double>> incomes;
        for (const Book &b : top5) {
            double income = m_incomeByBookId.value(b.id, b.price * b.totalSales); // fallback if server didn't send it
            incomes.push_back({b, income});
            total += income;
        }

        auto *series = new QPieSeries;
        // clear old legend rows
        while (m_pieLegendLayout->count() > 1) {
            QLayoutItem *it = m_pieLegendLayout->takeAt(0);
            if (it->widget()) it->widget()->deleteLater();
            delete it;
        }
        for (int i = 0; i < incomes.size(); ++i) {
            const int pct = total > 0 ? qRound(incomes[i].second / total * 100.0) : 0;
            QPieSlice *slice = series->append(QString::number(pct) + "%", incomes[i].second);
            slice->setColor(QColor(palette[i % palette.size()]));
            slice->setLabelVisible(true);
            slice->setLabelPosition(QPieSlice::LabelInsideHorizontal);
            slice->setLabelColor(Qt::white);
            slice->setBorderWidth(0);

            auto *row = new QLabel(QString(
                                       "<span style='color:%1;font-size:14px;'>●</span> "
                                       "<span style='color:#EAEAEA;font-size:12px;'>%2</span> "
                                       "<span style='color:%3;font-size:12px;'><b>$%4</b> (%5%)</span>")
                                       .arg(palette[i % palette.size()], incomes[i].first.title,
                                            kTextDim, QString::number(incomes[i].second, 'f', 2), QString::number(pct)));
            row->setStyleSheet("border:none;background:transparent;");
            m_pieLegendLayout->insertWidget(m_pieLegendLayout->count() - 1, row);
        }

        QChart *chart = makeDarkChart();
        chart->addSeries(series);
        replaceChart(m_pieView, chart);
    }
}

void PublisherPanel::updateSalesTrend(const QJsonArray &pts)
{
    auto *line = new QLineSeries;
    line->setColor(QColor(kAccent));
    line->setPointsVisible(true);
    QPen pen{QColor(kAccent)};
    pen.setWidth(2);
    line->setPen(pen);

    auto *axX = new QCategoryAxis;
    axX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    double maxV = 0;
    for (int i = 0; i < pts.size(); ++i) {
        const QJsonObject p = pts[i].toObject();
        const double sales = p["sales"].toDouble();
        line->append(i, sales);
        maxV = qMax(maxV, sales);

        QString label = p["period"].toString();               // "2026-07", "2026-W28" or "2026-07-14"
        const QDate d = QDate::fromString(label + "-01", "yyyy-MM-dd");
        if (d.isValid()) label = d.toString("MMM");           // monthly → "Jul"
        axX->append(label, i);
    }
    axX->setRange(-0.2, pts.size() - 0.8);
    styleAxis(axX);
    auto *base = new QLineSeries;
    for (int i = 0; i < pts.size(); ++i) base->append(i, 0);
    auto *area = new QAreaSeries(line, base);
    QLinearGradient g(QPointF(0, 0), QPointF(0, 1));
    g.setCoordinateMode(QGradient::ObjectBoundingMode);
    g.setColorAt(0.0, QColor(168, 92, 240, 140));
    g.setColorAt(1.0, QColor(168, 92, 240, 10));
    area->setBrush(g);
    area->setPen(Qt::NoPen);

    QChart *chart = makeDarkChart();
    chart->addSeries(area);
    chart->addSeries(line);
    auto *axY = new QValueAxis; axY->setRange(0, maxV * 1.15); axY->setLabelFormat("%d"); styleAxis(axY);
    chart->addAxis(axX, Qt::AlignBottom); chart->addAxis(axY, Qt::AlignLeft);
    area->attachAxis(axX); area->attachAxis(axY);
    line->attachAxis(axX); line->attachAxis(axY);
    replaceChart(m_trendView, chart);
}

void PublisherPanel::requestSalesTrend()
{
    QJsonObject req;
    req["action"] = "publisher_get_sales_trend";
    req["publisherId"] = m_publisherId;
    req["granularity"] = m_trendCombo ? m_trendCombo->currentText().toLower() : "monthly";
    sendRequest(req);
}


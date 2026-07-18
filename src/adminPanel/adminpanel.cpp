#include "adminpanel.h"
#include <QHBoxLayout>
#include <QVBoxLayout>

AdminPanel::AdminPanel(QWidget *parent)
    : QWidget(parent)
{
    m_socket = new QTcpSocket(this);

    setupUi();
    switchPage(0);

    connect(m_socket,&QTcpSocket::readyRead,this,&AdminPanel::onReadyRead);
    m_socket->connectToHost("127.0.0.1" , 1234);

    connect(m_socket, &QTcpSocket::connected, this, [this]() {

        m_userTab->refreshTable();
        m_booksTab->refreshTable();
        m_publishersTab->refreshTable();
        m_reviewsTab->refreshTable();
    });
}

void AdminPanel::setupUi()
{
    this->resize(800, 500);
    this->setStyleSheet("background-color: #060508; color: #EAEAEA; font-family: 'Segoe UI', Arial;");

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget *sidebar = new QWidget(this);
    sidebar->setFixedWidth(220);
    sidebar->setStyleSheet("background-color: #120E14; border-left: 1px solid #1F1724;");
    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(15, 25, 15, 25);
    sidebarLayout->setSpacing(12);

    QLabel *avatarLabel = new QLabel("👤", sidebar);
    avatarLabel->setStyleSheet("font-size: 40px; border: none; background: transparent;");
    avatarLabel->setAlignment(Qt::AlignCenter);

    QLabel *nameLabel = new QLabel("Admin", sidebar);
    nameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #FFEAD2; border: none; background: transparent;");
    nameLabel->setAlignment(Qt::AlignCenter);

    QLabel *roleLabel = new QLabel("System Administrator", sidebar);
    roleLabel->setStyleSheet("font-size: 11px; color: #A594B3; border: none; background: transparent;");
    roleLabel->setAlignment(Qt::AlignCenter);

    sidebarLayout->addWidget(avatarLabel);
    sidebarLayout->addWidget(nameLabel);
    sidebarLayout->addWidget(roleLabel);
    sidebarLayout->addSpacing(15);

    m_btnMonitor = new QPushButton("📈 Server Monitor", sidebar);
    m_btnUsers = new QPushButton("👥 Manage Users", sidebar);
    m_btnBooks = new QPushButton("📚 Manage Books", sidebar);
    m_btnPublishers = new QPushButton("🧑‍💻 Manage Publishers", sidebar);
    m_btnReviews = new QPushButton("🛡️ Moderate Reviews", sidebar);

    m_btnLogout = new QPushButton("🚪 Logout", sidebar);
    m_btnLogout->setCursor(Qt::PointingHandCursor);
    m_btnLogout->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #7C3E66; border-radius: 8px; padding: 8px; font-weight: bold; color: #D9C2D1; text-align: left; padding-left: 12px; }"
        "QPushButton:hover { background-color: rgba(124, 62, 102, 60); color: white; border: 1px solid #B06B96; }"
        );

    QString menuBtnStyle =
        "QPushButton { background-color: transparent; border: none; border-radius: 8px; padding: 10px; font-size: 13px; color: #9A8FA0; text-align: left; padding-left: 12px; }"
        "QPushButton:hover { background-color: #1F1724; color: #EAEAEA; }";

    m_btnMonitor->setStyleSheet(menuBtnStyle);
    m_btnUsers->setStyleSheet(menuBtnStyle);
    m_btnBooks->setStyleSheet(menuBtnStyle);
    m_btnPublishers->setStyleSheet(menuBtnStyle);
    m_btnReviews->setStyleSheet(menuBtnStyle);

    m_btnMonitor->setCursor(Qt::PointingHandCursor);
    m_btnUsers->setCursor(Qt::PointingHandCursor);
    m_btnBooks->setCursor(Qt::PointingHandCursor);
    m_btnPublishers->setCursor(Qt::PointingHandCursor);
    m_btnReviews->setCursor(Qt::PointingHandCursor);

    sidebarLayout->addWidget(m_btnMonitor);
    sidebarLayout->addWidget(m_btnUsers);
    sidebarLayout->addWidget(m_btnPublishers);
    sidebarLayout->addWidget(m_btnBooks);
    sidebarLayout->addWidget(m_btnReviews);

    sidebarLayout->addStretch();
    sidebarLayout->addWidget(m_btnLogout);

    m_stackedWidget = new QStackedWidget(this);

    ServerWindow *serverPage = new ServerWindow(this);
    m_stackedWidget->addWidget(serverPage);

    m_userTab = new UsersTab(m_socket,this);
    m_publishersTab = new PublishersTab(m_socket,this);
    m_booksTab = new BookTab(m_socket, this);
    m_reviewsTab = new ReviewsTab(m_socket, this);

    m_stackedWidget->addWidget(m_userTab);
    m_stackedWidget->addWidget(m_publishersTab);
    m_stackedWidget->addWidget(m_booksTab);
    m_stackedWidget->addWidget(m_reviewsTab);

    mainLayout->addWidget(m_stackedWidget);
    mainLayout->addWidget(sidebar);

    connect(m_btnMonitor, &QPushButton::clicked, this, [this](){ switchPage(0); });
    connect(m_btnUsers, &QPushButton::clicked, this, [this](){ switchPage(1); });
    connect(m_btnPublishers, &QPushButton::clicked, this, [this](){ switchPage(2); });
    connect(m_btnBooks, &QPushButton::clicked, this, [this](){ switchPage(3); });
    connect(m_btnReviews, &QPushButton::clicked, this, [this](){ switchPage(4); });
    connect(m_btnLogout, &QPushButton::clicked, this, &AdminPanel::logoutRequested);
}

void AdminPanel::switchPage(int index)
{
    m_stackedWidget->setCurrentIndex(index);
    updateButtonStyles(index);
}

void AdminPanel::updateButtonStyles(int currentIndex)
{
    QString normalStyle =
        "QPushButton { background-color: transparent; border: none; border-radius: 8px; padding: 12px; font-size: 14px; color: #9A8FA0; text-align: left; padding-left: 15px; }"
        "QPushButton:hover { background-color: #1F1724; color: #EAEAEA; }";

    QString activeStyle =
        "QPushButton { background-color: #7C3E66; border: none; border-radius: 8px; padding: 12px; font-size: 14px; font-weight: bold; color: #FFFFFF; text-align: left; padding-left: 15px; }";

    m_btnMonitor->setStyleSheet(currentIndex == 0 ? activeStyle : normalStyle);
    m_btnUsers->setStyleSheet(currentIndex == 1 ? activeStyle : normalStyle);
    m_btnPublishers->setStyleSheet(currentIndex == 2 ? activeStyle : normalStyle);
    m_btnBooks->setStyleSheet(currentIndex == 3 ? activeStyle : normalStyle);
    m_btnReviews->setStyleSheet(currentIndex == 4 ? activeStyle : normalStyle);
}

void AdminPanel::mousePressEvent(QMouseEvent *event)
{
    m_userTab->clearTableSelection();
    m_booksTab->clearTableSelection();;
    m_publishersTab->clearTableSelection();
    m_reviewsTab->clearTableSelection();
    QWidget::mousePressEvent(event);
}

void AdminPanel::onReadyRead()
{
    while (m_socket->canReadLine()) {
        QByteArray line = m_socket->readLine().trimmed();
        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (doc.isNull()) continue;

        QJsonObject response = doc.object();

        m_userTab->handleServerResponse(response);
        m_booksTab->handleServerResponse(response);
        m_publishersTab->handleServerResponse(response);
        m_reviewsTab->handleServerResponse(response);
    }
}
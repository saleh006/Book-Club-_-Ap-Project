#ifndef ADMINPANEL_H
#define ADMINPANEL_H

#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QLineEdit>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include "serverwindow.h"

class AdminPanel : public QWidget
{
    Q_OBJECT
public:
    explicit AdminPanel(QWidget *parent = nullptr);

signals:
    void logoutRequested();

private slots:
    void switchPage(int index);

    void filterUsers(const QString &text);
    void handleBlockUser();
    void handleUnblockUser();
    void handleViewUserDetails();
    void handleDeleteUser();

    void filterBooks(const QString &text);
    void handleApproveBook();
    void handleRejectBook();
    void handleDeleteBook();
    void handleViewBookDetails();
    void handleEditBook();

    void filterPublishers(const QString &text);
    void handleBlockPublisher();
    void handleUnblockPublisher();
    void handleViewPublisherDetails();
    void handleDeletePublisher();

    void filterReviews(const QString &text);
    void handleApproveReview();
    void handleReviewFilterChanged(int index);
    void handleDeleteReview();
    void populateReviewsTable(const QJsonArray &reviews, bool pendingOnly);

    void onReadyRead();

private:
    QTcpSocket *m_socket;
    void updateRowAppearance(QTableWidget *table, int row, bool isDimmed);

    QStackedWidget *m_stackedWidget;
    QPushButton *m_btnMonitor;
    QPushButton *m_btnUsers;
    QPushButton *m_btnBooks;
    QPushButton *m_btnLogout;
    QPushButton *m_btnPublishers;
    QPushButton *m_btnReviews;

    QTableWidget *m_usersTable;
    QLineEdit *m_searchEdit;
    QPushButton *m_btnBlock;
    QPushButton *m_btnUnblock;
    QWidget* createUsersPage();
    QPushButton *m_btnUserDetails;
    QPushButton *m_btnDeleteUser;

    QTableWidget *m_booksTable;
    QLineEdit *m_bookSearchEdit;
    QPushButton *m_btnApprove;
    QPushButton *m_btnReject;
    QPushButton *m_btnBookDetails;
    QPushButton *m_btnEditBook;
    QString m_pendingBookDetailPurpose;
    QWidget* createBooksPage();
    QPushButton *m_btnDeleteBook;

    QTableWidget *m_publishersTable;
    QLineEdit *m_publisherSearchEdit;
    QPushButton *m_btnBlockPublisher;
    QPushButton *m_btnUnblockPublisher;
    QPushButton *m_btnPublisherDetails;
    QWidget* createPublishersPage();
    QPushButton *m_btnDeletePublisher;

    QLineEdit *m_reviewSearchEdit;
    QTableWidget *m_reviewsTable;
    QPushButton *m_btnDeleteReview;
    QComboBox *m_reviewFilterCombo;
    QPushButton *m_btnApproveReview;
    QWidget* createReviewsPage();
    bool m_reviewShowPendingOnly = false;


    void setupUi();
    void updateButtonStyles(int currentIndex);
    void showUserDetailsDialog(const QJsonObject &data);
    void showPublisherDetailsDialog(const QJsonObject &data);
    void showBookDetailsDialog(const QJsonObject &data);
    void refreshUsersTable();
    void refreshBooksTable();
    void refreshPublishersTable();
    void refreshReviewsTable();
protected:
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // ADMINPANEL_H
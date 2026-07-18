#ifndef REVIEWSTAB_H
#define REVIEWSTAB_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <QLabel>

class ReviewsTab : public QWidget
{
    Q_OBJECT
public:
    explicit ReviewsTab(QTcpSocket *socket, QWidget *parent = nullptr);

    void refreshTable();
    void clearTableSelection();

public slots:
    void handleServerResponse(const QJsonObject &response);

private slots:
    void filterReviews(const QString &text);
    void handleApproveReview();
    void handleReviewFilterChanged(int index);
    void handleDeleteReview();

private:
    QWidget* setupUi();
    void populateReviewsTable(const QJsonArray &reviews, bool pendingOnly);

    QTcpSocket *m_socket;

    QLineEdit *m_reviewSearchEdit;
    QTableWidget *m_reviewsTable;
    QPushButton *m_btnDeleteReview;
    QComboBox *m_reviewFilterCombo;
    QPushButton *m_btnApproveReview;
    bool m_reviewShowPendingOnly = false;
};

#endif // REVIEWSTAB_H

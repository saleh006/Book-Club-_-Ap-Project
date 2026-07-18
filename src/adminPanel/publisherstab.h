#ifndef PUBLISHERSTAB_H
#define PUBLISHERSTAB_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTcpSocket>
#include <QJsonObject>
#include <QLabel>

class PublishersTab : public QWidget
{
    Q_OBJECT
public:
    explicit PublishersTab(QTcpSocket *socket, QWidget *parent = nullptr);

    void refreshTable();
    void clearTableSelection();

public slots:
    void handleServerResponse(const QJsonObject &response);

private slots:
    void filterPublishers(const QString &text);
    void handleBlockPublisher();
    void handleUnblockPublisher();
    void handleViewPublisherDetails();
    void handleDeletePublisher();

private:
    QWidget* setupUi();
    void showPublisherDetailsDialog(const QJsonObject &data);
    void setRowDimmed(QTableWidget *table, int row, bool isDimmed);

    QTcpSocket *m_socket;

    QTableWidget *m_publishersTable;
    QLineEdit *m_publisherSearchEdit;
    QPushButton *m_btnBlockPublisher;
    QPushButton *m_btnUnblockPublisher;
    QPushButton *m_btnPublisherDetails;
    QPushButton *m_btnDeletePublisher;
};

#endif // PUBLISHERSTAB_H

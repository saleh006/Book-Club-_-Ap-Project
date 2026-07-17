#ifndef USERSTAB_H
#define USERSTAB_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTcpSocket>
#include <QJsonObject>
#include <QLabel>

class UsersTab : public QWidget
{
    Q_OBJECT
public:
    explicit UsersTab(QTcpSocket *socket, QWidget *parent = nullptr);

    void refreshTable();
    void clearTableSelection();

public slots:
    void handleServerResponse(const QJsonObject &response);

private slots:
    void filterUsers(const QString &text);
    void handleBlockUser();
    void handleUnblockUser();
    void handleViewUserDetails();
    void handleDeleteUser();

private:
    QWidget* setupUi();
    void showUserDetailsDialog(const QJsonObject &data);
    void setRowDimmed(QTableWidget*, int, bool);

    QTcpSocket *m_socket;

    QTableWidget *m_usersTable;
    QLineEdit *m_searchEdit;
    QPushButton *m_btnBlock;
    QPushButton *m_btnUnblock;
    QPushButton *m_btnUserDetails;
    QPushButton *m_btnDeleteUser;
};

#endif // USERSTAB_H

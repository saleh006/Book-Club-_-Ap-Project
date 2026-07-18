#ifndef BOOKTAB_H
#define BOOKTAB_H

#include <QDialog>
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTcpSocket>
#include <QJsonObject>
#include <QLabel>
#include "models.h"

class QTextEdit;
class QDoubleSpinBox;

class AddEditBookDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddEditBookDialog(const Book &existingBook, QWidget *parent = nullptr);
    Book resultBook() const;

private:
    void setupUi(bool isEditMode);

    Book m_book;
    QLineEdit *m_titleEdit;
    QLineEdit *m_authorEdit;
    QLineEdit *m_genreEdit;
    QTextEdit *m_descriptionEdit;
    QDoubleSpinBox *m_priceSpin;
    QLineEdit *m_coverPathEdit;
    QLineEdit *m_pdfPathEdit;
};

class BookTab : public QWidget
{
    Q_OBJECT
public:
    explicit BookTab(QTcpSocket *socket, QWidget *parent = nullptr);

    void refreshTable();
    void clearTableSelection();

public slots:
    void handleServerResponse(const QJsonObject &response);

private slots:
    void filterBooks(const QString &text);
    void handleApproveBook();
    void handleRejectBook();
    void handleDeleteBook();
    void handleViewBookDetails();
    void handleEditBook();

private:
    QWidget* setupUi();
    void showBookDetailsDialog(const QJsonObject &data);
    void setRowDimmed(QTableWidget*, int, bool);

    QTcpSocket *m_socket;

    QTableWidget *m_booksTable;
    QLineEdit *m_bookSearchEdit;
    QPushButton *m_btnApprove;
    QPushButton *m_btnReject;
    QPushButton *m_btnBookDetails;
    QPushButton *m_btnEditBook;
    QPushButton *m_btnDeleteBook;
    QString m_pendingBookDetailPurpose;
};

#endif // BOOKTAB_H

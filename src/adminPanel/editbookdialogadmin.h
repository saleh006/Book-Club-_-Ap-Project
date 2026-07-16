#ifndef ADDEDITBOOKDIALOG_H
#define ADDEDITBOOKDIALOG_H

#include <QDialog>
//#include "models.h"

struct Book {
    int id = -1;
    int publisherId = -1;
    QString title;
    QString author;
    QString genre;
    QString description;
    double price = 0.0;
    QString coverImagePath;
    QString pdfPath;
    int status = 1;
    double averageRating = 0.0;
    int totalSales = 0;
};

class QLineEdit;
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

#endif // ADDEDITBOOKDIALOG_H
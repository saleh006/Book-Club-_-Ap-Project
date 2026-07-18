#ifndef ADDEDITBOOKDIALOG_H
#define ADDEDITBOOKDIALOG_H

#include <QDialog>
#include "models.h"

class QLineEdit;
class QTextEdit;
class QDoubleSpinBox;

class AddEditBookDialogPub : public QDialog
{
    Q_OBJECT

public:
    explicit AddEditBookDialogPub(const Book &existingBook, QWidget *parent = nullptr);
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
#ifndef SETOFFERDIALOG_H
#define SETOFFERDIALOG_H

#include <QDialog>
#include "models.h"

class QComboBox;
class QDoubleSpinBox;
class QDateTimeEdit;

class SetOfferDialog : public QDialog
{
    Q_OBJECT

public:
    // Create a new offer for a book.
    explicit SetOfferDialog(int bookId, QWidget *parent = nullptr);

    // Edit an existing discount: pre-fills type/value/start/end from `existing`
    SetOfferDialog(int bookId, const Discount &existing, QWidget *parent = nullptr);

    Discount resultDiscount() const;

private:
    void setupUi();
    void prefill(const Discount &existing);

    int m_bookId;
    int m_discountId = -1;

    QComboBox *m_typeCombo;
    QDoubleSpinBox *m_valueSpin;
    QDateTimeEdit *m_startEdit;
    QDateTimeEdit *m_endEdit;
};

#endif // SETOFFERDIALOG_H
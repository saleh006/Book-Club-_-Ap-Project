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
    explicit SetOfferDialog(int bookId, QWidget *parent = nullptr);
    Discount resultDiscount() const;

private:
    void setupUi();

    int m_bookId;
    QComboBox *m_typeCombo;
    QDoubleSpinBox *m_valueSpin;
    QDateTimeEdit *m_startEdit;
    QDateTimeEdit *m_endEdit;
};

#endif // SETOFFERDIALOG_H
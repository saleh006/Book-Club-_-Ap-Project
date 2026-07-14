#include "setofferdialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QLabel>

SetOfferDialog::SetOfferDialog(int bookId, QWidget *parent)
    : QDialog(parent), m_bookId(bookId)
{
    setupUi();
}

void SetOfferDialog::setupUi()
{
    setWindowTitle("Set Offer");
    setFixedWidth(380);
    setStyleSheet(
        "QDialog { background-color: #120E14; color: #EAEAEA; }"
        "QLabel { color: #A594B3; background-color: #120E14;}"
        "QComboBox, QDoubleSpinBox, QDateTimeEdit {"
        "   background-color: #1F1724; border: 1px solid #5F2E4F;"
        "   border-radius: 6px; padding: 6px; color: #EAEAEA; }"
        "QComboBox:focus, QDoubleSpinBox:focus, QDateTimeEdit:focus { border: 1px solid #7C3E66; }"
        "QPushButton { background-color: #7C3E66; border: none; border-radius: 6px; padding: 8px 16px; color: white; font-weight: bold; }"
        "QPushButton:hover { background-color: #5F2E4F; }"
        );

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem("Percent Off", "percent");
    m_typeCombo->addItem("Flat Amount Off", "flat");

    m_valueSpin = new QDoubleSpinBox(this);
    m_valueSpin->setRange(0.0, 100000.0);
    m_valueSpin->setDecimals(2);
    m_valueSpin->setValue(10.0);

    m_startEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    m_startEdit->setCalendarPopup(true);
    m_startEdit->setDisplayFormat("yyyy-MM-dd HH:mm");

    m_endEdit = new QDateTimeEdit(QDateTime::currentDateTime().addDays(7), this);
    m_endEdit->setCalendarPopup(true);
    m_endEdit->setDisplayFormat("yyyy-MM-dd HH:mm");

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow("Discount Type", m_typeCombo);
    formLayout->addRow("Value", m_valueSpin);
    formLayout->addRow("Starts", m_startEdit);
    formLayout->addRow("Ends", m_endEdit);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(m_typeCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_valueSpin->setPrefix("");
        m_valueSpin->setSuffix("");

        if (m_typeCombo->itemData(index).toString() == "percent") {
            m_valueSpin->setSuffix(" %");
            m_valueSpin->setRange(0.0, 100.0);
        } else {
            m_valueSpin->setPrefix("$ ");
            m_valueSpin->setRange(0.0, 100000.0);
        }
    });
    m_valueSpin->setSuffix(" %");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);
}

Discount SetOfferDialog::resultDiscount() const
{
    Discount d;
    d.bookId = m_bookId;
    d.type = m_typeCombo->currentData().toString();
    d.value = m_valueSpin->value();
    d.startDate = m_startEdit->dateTime();
    d.endDate = m_endEdit->dateTime();
    return d;
}
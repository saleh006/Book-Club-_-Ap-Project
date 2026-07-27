#include "statisticscard.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

StatisticsCard::StatisticsCard(const QString &icon, const QString &value,
                               const QString &label, QWidget *parent)
    : HoverCard(parent)
{
    setHoverEnabled(false);
    setBackgroundColor(QColor(0, 0, 0, 60));
    setBorderColors(QColor(255, 255, 255, 20), QColor(255, 255, 255, 20));
    setCornerRadius(14);
    setMinimumHeight(64);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(14, 10, 16, 10);
    row->setSpacing(12);

    auto *iconBubble = new QLabel(icon, this);
    iconBubble->setFixedSize(38, 38);
    iconBubble->setAlignment(Qt::AlignCenter);
    iconBubble->setAttribute(Qt::WA_TransparentForMouseEvents);
    iconBubble->setStyleSheet(
        "background-color: rgba(255,255,255,18); border-radius: 10px; font-size: 16px;"
        "background: rgba(255,255,255,18); border: none;");

    auto *textCol = new QVBoxLayout;
    textCol->setSpacing(0);
    textCol->setContentsMargins(0, 0, 0, 0);

    m_valueLabel = new QLabel(value, this);
    m_valueLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_valueLabel->setStyleSheet(
        "color: #FFFFFF; font-size: 19px; font-weight: 700; background: transparent; border: none;");

    auto *labelLbl = new QLabel(label, this);
    labelLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
    labelLbl->setStyleSheet(
        "color: rgba(255,255,255,150); font-size: 11px; background: transparent; border: none;");

    textCol->addWidget(m_valueLabel);
    textCol->addWidget(labelLbl);

    row->addWidget(iconBubble);
    row->addLayout(textCol, 1);
}

void StatisticsCard::setValue(const QString &value)
{
    m_valueLabel->setText(value);
}

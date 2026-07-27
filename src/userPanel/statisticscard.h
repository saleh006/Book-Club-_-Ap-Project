#ifndef STATISTICSCARD_H
#define STATISTICSCARD_H

#include "hovercard.h"
#include <QString>

class QLabel;

class StatisticsCard : public HoverCard
{
    Q_OBJECT
public:
    explicit StatisticsCard(const QString &icon, const QString &value,
                            const QString &label, QWidget *parent = nullptr);

    void setValue(const QString &value);

private:
    QLabel *m_valueLabel = nullptr;
};

#endif // STATISTICSCARD_H

#ifndef HOVERCARD_H
#define HOVERCARD_H

#include <QFrame>
#include <QColor>

class QPropertyAnimation;
class QGraphicsDropShadowEffect;

// Reusable "premium card" surface used throughout the My Library page
// (BookCard, ShelfCard, ContinueReadingCard, StatisticsCard, AddShelfCard).
//
// Qt stylesheets cannot animate a `transition`, so this widget paints its
// own rounded background/border in paintEvent() and exposes the border
// color as a Q_PROPERTY. That lets us drive a real QPropertyAnimation on
// hover (150ms border-color + shadow-elevation transition) instead of the
// instant QSS swap used elsewhere for simple buttons.
//
// Non-interactive children (QLabel) placed on top should have
// Qt::WA_TransparentForMouseEvents set so a click on them still reaches
// this widget's mousePressEvent/clicked() signal; interactive children
// (QPushButton) keep receiving their own clicks normally.
class HoverCard : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor)

public:
    explicit HoverCard(QWidget *parent = nullptr);

    void setBackgroundColor(const QColor &color);
    void setBorderColors(const QColor &normal, const QColor &hover);
    void setCornerRadius(int radius);
    void setHoverEnabled(bool enabled);

    QColor borderColor() const { return m_borderColor; }
    void setBorderColor(const QColor &color);

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    int cornerRadius() const { return m_radius; }

private:
    QColor m_backgroundColor;
    QColor m_normalBorder;
    QColor m_hoverBorder;
    QColor m_borderColor;
    int m_radius = 18;
    bool m_hoverEnabled = true;
    bool m_pressed = false;

    QPropertyAnimation *m_borderAnim = nullptr;
    QPropertyAnimation *m_shadowAnim = nullptr;
    QGraphicsDropShadowEffect *m_shadow = nullptr;
};

#endif // HOVERCARD_H

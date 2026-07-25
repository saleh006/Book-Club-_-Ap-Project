#include "hovercard.h"

#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QMouseEvent>
#include <QEnterEvent>

HoverCard::HoverCard(QWidget *parent)
    : QFrame(parent)
{
    setAttribute(Qt::WA_Hover, true);
    setFrameShape(QFrame::NoFrame);
    // The frame paints its own rounded background/border in paintEvent();
    // this stylesheet just makes sure Qt doesn't draw anything underneath it.
    setStyleSheet("background: transparent; border: none;");

    m_backgroundColor = QColor("#181320");
    m_normalBorder    = QColor(255, 255, 255, 20);
    m_hoverBorder     = QColor("#A855F7");
    m_borderColor     = m_normalBorder;

    m_shadow = new QGraphicsDropShadowEffect(this);
    m_shadow->setColor(QColor(0, 0, 0, 130));
    m_shadow->setBlurRadius(18);
    m_shadow->setOffset(0, 6);
    setGraphicsEffect(m_shadow);

    m_borderAnim = new QPropertyAnimation(this, "borderColor", this);
    m_borderAnim->setDuration(150);

    m_shadowAnim = new QPropertyAnimation(m_shadow, "blurRadius", this);
    m_shadowAnim->setDuration(150);
}

void HoverCard::setBackgroundColor(const QColor &color)
{
    m_backgroundColor = color;
    update();
}

void HoverCard::setBorderColors(const QColor &normal, const QColor &hover)
{
    m_normalBorder = normal;
    m_hoverBorder = hover;
    m_borderColor = normal;
    update();
}

void HoverCard::setCornerRadius(int radius)
{
    m_radius = radius;
    update();
}

void HoverCard::setHoverEnabled(bool enabled)
{
    m_hoverEnabled = enabled;
    if (!enabled)
        setCursor(Qt::ArrowCursor);
}

void HoverCard::setBorderColor(const QColor &color)
{
    m_borderColor = color;
    update();
}

void HoverCard::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = rect().adjusted(1, 1, -1, -1);
    QPainterPath path;
    path.addRoundedRect(r, m_radius, m_radius);

    p.fillPath(path, m_backgroundColor);
    p.setPen(QPen(m_borderColor, 1.4));
    p.drawPath(path);
}

void HoverCard::enterEvent(QEnterEvent *event)
{
    if (m_hoverEnabled) {
        setCursor(Qt::PointingHandCursor);

        m_borderAnim->stop();
        m_borderAnim->setStartValue(m_borderColor);
        m_borderAnim->setEndValue(m_hoverBorder);
        m_borderAnim->start();

        m_shadowAnim->stop();
        m_shadowAnim->setStartValue(m_shadow->blurRadius());
        m_shadowAnim->setEndValue(30.0);
        m_shadowAnim->start();
        m_shadow->setOffset(0, 10);
    }
    QFrame::enterEvent(event);
}

void HoverCard::leaveEvent(QEvent *event)
{
    if (m_hoverEnabled) {
        m_borderAnim->stop();
        m_borderAnim->setStartValue(m_borderColor);
        m_borderAnim->setEndValue(m_normalBorder);
        m_borderAnim->start();

        m_shadowAnim->stop();
        m_shadowAnim->setStartValue(m_shadow->blurRadius());
        m_shadowAnim->setEndValue(18.0);
        m_shadowAnim->start();
        m_shadow->setOffset(0, 6);
    }
    QFrame::leaveEvent(event);
}

void HoverCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_pressed = true;
    QFrame::mousePressEvent(event);
}

void HoverCard::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_pressed && event->button() == Qt::LeftButton) {
        m_pressed = false;
        if (rect().contains(event->pos()))
            emit clicked();
    }
    QFrame::mouseReleaseEvent(event);
}

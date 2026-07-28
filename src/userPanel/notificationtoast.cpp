#include "notificationtoast.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QEasingCurve>
#include <QTimer>


struct ToastVisual { QString icon; QString accent; };

static ToastVisual toastVisualFor(const QString &title)
{
    if (title.startsWith(QStringLiteral("New in"), Qt::CaseInsensitive))
        return { QStringLiteral("📘"), QStringLiteral("#6C8EF5") };
    if (title.compare(QStringLiteral("Price Drop!"), Qt::CaseInsensitive) == 0)
        return { QStringLiteral("🏷"), QStringLiteral("#45C48A") };
    if (title.compare(QStringLiteral("Book Sold!"), Qt::CaseInsensitive) == 0)
        return { QStringLiteral("💰"), QStringLiteral("#F0B429") };
    if (title.compare(QStringLiteral("New Review"), Qt::CaseInsensitive) == 0)
        return { QStringLiteral("⭐"), QStringLiteral("#F2994A") };
    return { QStringLiteral("🔔"), QStringLiteral("#B06B96") };
}


void NotificationToast::show(QWidget *hostWidget, const QString &title, const QString &message)
{
    if (!hostWidget) return;
    auto *toast = new NotificationToast(hostWidget, title, message);
    toast->playInThenOut();
}

NotificationToast::NotificationToast(QWidget *hostWidget, const QString &title, const QString &message)
    : QWidget(hostWidget), m_host(hostWidget)
{
    const ToastVisual visual = toastVisualFor(title);

    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_DeleteOnClose);

    setAttribute(Qt::WA_StyledBackground, true);

    setFixedWidth(340);

    setStyleSheet(QString(
        "NotificationToast {"
        "  background-color: rgba(15, 20, 30, 0.95);"
        "  border: 1px solid #2B3545;"
        "  border-bottom: 3px solid %1;"
        "  border-radius: 12px;"
        "}").arg(visual.accent + "20"));

    auto *outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(16, 14, 16, 14);
    outerLayout->setSpacing(14);

    auto *iconBubble = new QLabel(visual.icon, this);
    iconBubble->setFixedSize(42, 42);
    iconBubble->setAlignment(Qt::AlignCenter);

    iconBubble->setStyleSheet(QString(
                                  "font-size: 20px; border-radius: 21px; background-color: %1; border: none;"
                                  ).arg(visual.accent + "20"));

    auto *textCol = new QVBoxLayout();
    textCol->setSpacing(4);

    auto *titleLbl = new QLabel(title, this);

    titleLbl->setStyleSheet("color: #FFFFFF; font-size: 14px; font-weight: bold; border: none; background: transparent;");
    titleLbl->setWordWrap(true);

    auto *msgLbl = new QLabel(message, this);

    msgLbl->setStyleSheet("color: #9CA3AF; font-size: 12px; font-weight: 500; border: none; background: transparent;");
    msgLbl->setWordWrap(true);

    textCol->addWidget(titleLbl);
    textCol->addWidget(msgLbl);

    auto *closeBtn = new QPushButton("✕", this);
    closeBtn->setFixedSize(24, 24);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton { color: #6B7280; border: none; background: transparent; font-size: 14px; border-radius: 12px; }"
        "QPushButton:hover { color: #FFFFFF; background-color: #2B3545; }"
        );
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);

    outerLayout->addWidget(iconBubble, 0, Qt::AlignTop);
    outerLayout->addLayout(textCol, 1);
    outerLayout->addWidget(closeBtn, 0, Qt::AlignTop);

    adjustSize();

    const int margin = 20;
    const QPoint startPos(m_host->width(), margin);
    const QPoint endPos(m_host->width() - width() - margin, margin);
    move(startPos);

    auto *opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(opacityEffect);
    opacityEffect->setOpacity(0.0);

    m_slideAnim = new QPropertyAnimation(this, "pos", this);
    m_slideAnim->setDuration(350);
    m_slideAnim->setStartValue(startPos);
    m_slideAnim->setEndValue(endPos);
    m_slideAnim->setEasingCurve(QEasingCurve::OutBack);

    m_fadeInAnim = new QPropertyAnimation(opacityEffect, "opacity", this);
    m_fadeInAnim->setDuration(300);
    m_fadeInAnim->setStartValue(0.0);
    m_fadeInAnim->setEndValue(1.0);
}

void NotificationToast::playInThenOut()
{
    QWidget::show();
    raise();
    m_slideAnim->start();
    m_fadeInAnim->start();

    QTimer::singleShot(5000, this, [this]() {
        auto *effect = qobject_cast<QGraphicsOpacityEffect *>(graphicsEffect());
        if (!effect) { close(); return; }
        auto *fadeOut = new QPropertyAnimation(effect, "opacity", this);
        fadeOut->setDuration(350);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        connect(fadeOut, &QPropertyAnimation::finished, this, &QWidget::close);
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

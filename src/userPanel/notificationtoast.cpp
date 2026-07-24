#include "notificationtoast.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QEasingCurve>
#include <QTimer>

void NotificationToast::show(QWidget *hostWidget, const QString &title, const QString &message)
{
    if (!hostWidget) return;
    auto *toast = new NotificationToast(hostWidget, title, message);
    toast->playInThenOut();
}

NotificationToast::NotificationToast(QWidget *hostWidget, const QString &title, const QString &message)
    : QWidget(hostWidget), m_host(hostWidget)
{
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_DeleteOnClose);
    setFixedWidth(320);
    setStyleSheet(
        "background-color: #1A1420;"
        "border: 1px solid #7C3E66;"
        "border-radius: 12px;");

    auto *outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(14, 12, 14, 12);
    outerLayout->setSpacing(10);

    auto *bell = new QLabel("\U0001F514", this); // 🔔
    bell->setStyleSheet("font-size:18px;border:none;background:transparent;");
    bell->setFixedWidth(24);

    auto *textCol = new QVBoxLayout();
    textCol->setSpacing(3);
    auto *titleLbl = new QLabel(title, this);
    titleLbl->setStyleSheet("color:#FFFFFF;font-size:13px;font-weight:bold;border:none;background:transparent;");
    titleLbl->setWordWrap(true);
    auto *msgLbl = new QLabel(message, this);
    msgLbl->setStyleSheet("color:#C9BFCB;font-size:12px;border:none;background:transparent;");
    msgLbl->setWordWrap(true);
    textCol->addWidget(titleLbl);
    textCol->addWidget(msgLbl);

    auto *closeBtn = new QPushButton("\u2715", this); // ✕
    closeBtn->setFixedSize(20, 20);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton{color:#9A8FA0;border:none;background:transparent;font-size:12px;}"
        "QPushButton:hover{color:#FFFFFF;}");
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);

    outerLayout->addWidget(bell);
    outerLayout->addLayout(textCol, 1);
    outerLayout->addWidget(closeBtn, 0, Qt::AlignTop);

    adjustSize();

    const int margin = 16;
    const QPoint startPos(m_host->width(), margin);
    const QPoint endPos(m_host->width() - width() - margin, margin);
    move(startPos);

    auto *opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(opacityEffect);
    opacityEffect->setOpacity(0.0);

    m_slideAnim = new QPropertyAnimation(this, "pos", this);
    m_slideAnim->setDuration(280);
    m_slideAnim->setStartValue(startPos);
    m_slideAnim->setEndValue(endPos);
    m_slideAnim->setEasingCurve(QEasingCurve::OutCubic);

    m_fadeInAnim = new QPropertyAnimation(opacityEffect, "opacity", this);
    m_fadeInAnim->setDuration(280);
    m_fadeInAnim->setStartValue(0.0);
    m_fadeInAnim->setEndValue(1.0);
}

void NotificationToast::playInThenOut()
{
    QWidget::show();
    raise();
    m_slideAnim->start();
    m_fadeInAnim->start();

    QTimer::singleShot(4000, this, [this]() {
        auto *effect = qobject_cast<QGraphicsOpacityEffect *>(graphicsEffect());
        if (!effect) { close(); return; }
        auto *fadeOut = new QPropertyAnimation(effect, "opacity", this);
        fadeOut->setDuration(320);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        connect(fadeOut, &QPropertyAnimation::finished, this, &QWidget::close);
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

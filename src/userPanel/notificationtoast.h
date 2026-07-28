#ifndef NOTIFICATIONTOAST_H
#define NOTIFICATIONTOAST_H

#include <QWidget>
#include <QDateTime>
#include <QFrame>
#include <QEvent>

class QPropertyAnimation;

namespace {
static const char *kNotifPanelBg    = "#121017";
static const char *kNotifCardBg     = "#1B1822";
static const char *kNotifCardBorder = "#2A2735";
static const char *kNotifUnreadBg   = "#211D2C";
static const char *kNotifTextDim    = "#8D8797";

struct NotifVisual { QString icon; QString accent; QString label; };

NotifVisual notificationVisualFor(const QString &title)
{
    if (title.startsWith(QStringLiteral("New in"), Qt::CaseInsensitive))
        return { QStringLiteral("\U0001F4D8"), QStringLiteral("#6C8EF5"), QStringLiteral("New Release") };
    if (title.compare(QStringLiteral("Price Drop!"), Qt::CaseInsensitive) == 0)
        return { QStringLiteral("\U0001F3F7\uFE0F"), QStringLiteral("#45C48A"), QStringLiteral("Discount") };
    if (title.compare(QStringLiteral("Book Sold!"), Qt::CaseInsensitive) == 0)
        return { QStringLiteral("\U0001F4B0"), QStringLiteral("#F0B429"), QStringLiteral("Sale") };
    if (title.compare(QStringLiteral("New Review"), Qt::CaseInsensitive) == 0)
        return { QStringLiteral("\U00002B50"), QStringLiteral("#F2994A"), QStringLiteral("Review") };
    return { QStringLiteral("\U0001F514"), QStringLiteral("#B06B96"), QStringLiteral("Update") };
}

QString formatRelativeNotifTime(const QDateTime &dt)
{
    if (!dt.isValid()) return QString();
    const qint64 secs = dt.secsTo(QDateTime::currentDateTime());
    if (secs < 0)      return dt.toString("MMM d, HH:mm");
    if (secs < 60)     return QStringLiteral("Just now");
    if (secs < 3600)   return QStringLiteral("%1m ago").arg(secs / 60);
    if (secs < 86400)  return QStringLiteral("%1h ago").arg(secs / 3600);
    if (secs < 172800) return QStringLiteral("Yesterday");
    if (secs < 604800) return QStringLiteral("%1d ago").arg(secs / 86400);
    return dt.toString("MMM d");
}

class NotifRowFrame : public QFrame
{
public:
    using QFrame::QFrame;
    QWidget *hoverButton = nullptr;
protected:
    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::Enter) { if (hoverButton) hoverButton->show(); }
        else if (e->type() == QEvent::Leave) { if (hoverButton) hoverButton->hide(); }
        return QFrame::event(e);
    }
};

}

class NotificationToast : public QWidget
{
    Q_OBJECT
public:
    static void show(QWidget *hostWidget, const QString &title, const QString &message);

private:
    explicit NotificationToast(QWidget *hostWidget, const QString &title, const QString &message);
    void playInThenOut();

    QWidget *m_host;
    QPropertyAnimation *m_slideAnim = nullptr;
    QPropertyAnimation *m_fadeInAnim = nullptr;
};

#endif // NOTIFICATIONTOAST_H

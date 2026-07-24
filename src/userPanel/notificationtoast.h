#ifndef NOTIFICATIONTOAST_H
#define NOTIFICATIONTOAST_H

#include <QWidget>

class QPropertyAnimation;

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

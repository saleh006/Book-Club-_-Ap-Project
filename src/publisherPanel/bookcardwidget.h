#ifndef BOOKCARDWIDGET_H
#define BOOKCARDWIDGET_H

#include <QWidget>
#include "models.h"

class QLabel;
class QPushButton;

class BookCardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BookCardWidget(const Book &book, QWidget *parent = nullptr);
    int bookId() const;

signals:
    void editRequested(int bookId);
    void deleteRequested(int bookId);
    void offerRequested(int bookId);
    void toggleActiveRequested(int bookId, int newStatus); // newStatus: 1=active, 0=inactive

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    Book m_book;
    QLabel *m_coverLabel;
    QLabel *m_titleLabel;
    QLabel *m_statusBadge;
    QWidget *m_overlay;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_offerBtn;
    QPushButton *m_toggleActiveBtn;
};

#endif // BOOKCARDWIDGET_H
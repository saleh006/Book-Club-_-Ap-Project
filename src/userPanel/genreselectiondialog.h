#ifndef GENRESELECTIONDIALOG_H
#define GENRESELECTIONDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QVector>

class QPushButton;
class QLabel;

class GenreSelectionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GenreSelectionDialog(QWidget *parent = nullptr);
    QStringList selectedGenres() const;

private:
    void updateContinueButton();

    QVector<QPushButton*> m_genreButtons;
    QPushButton *m_continueBtn = nullptr;
    QLabel *m_counterLabel = nullptr;
};

#endif // GENRESELECTIONDIALOG_H
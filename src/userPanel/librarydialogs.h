#ifndef LIBRARYDIALOGS_H
#define LIBRARYDIALOGS_H

#include <QDialog>
#include <QColor>
#include <QVector>
#include "shelfcard.h"   // ShelfSummary, shelfIconForTitle

class QLineEdit;
class QPlainTextEdit;
class QButtonGroup;
class QPushButton;
class QAbstractButton;

// "Create New Shelf" / "Edit Shelf" dialog: name, optional description,
// and a row of color swatches. The same dialog is reused for editing an
// existing shelf (editMode=true, fields pre-filled).
class CreateShelfDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CreateShelfDialog(QWidget *parent = nullptr,
                               bool editMode = false,
                               const QString &existingName = QString(),
                               const QString &existingDescription = QString(),
                               const QColor &existingColor = QColor("#A855F7"));

    QString shelfName() const;
    QString shelfDescription() const;
    QColor selectedColor() const { return m_selectedColor; }

private:
    QPushButton *makeSwatch(const QColor &color);

    QLineEdit *m_nameEdit = nullptr;
    QPlainTextEdit *m_descEdit = nullptr;
    QButtonGroup *m_colorGroup = nullptr;
    QColor m_selectedColor;
};

// "Move to Shelf" dialog: a radio-button list of the user's shelves plus a
// shortcut to create a brand-new shelf on the fly.
class MoveToShelfDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MoveToShelfDialog(const QVector<ShelfSummary> &shelves,
                               int currentShelfId,
                               QWidget *parent = nullptr);

    int selectedShelfId() const { return m_selectedShelfId; }

signals:
    void createNewShelfRequested();

private:
    QButtonGroup *m_group = nullptr;
    int m_selectedShelfId = -1;
};

#endif // LIBRARYDIALOGS_H

#include "styledmessagebox.h"
#include <QMessageBox>
#include <QPixmap>
#include <QLabel>
#include <QDebug>

const QString kGlobalMessageBoxStyle = R"(
    QMessageBox {
        background-color: #14121B;
        border: 1px solid #26212E;
        border-radius: 16px;
    }
    QMessageBox QLabel {
        color: #A9A3B3;
        font-family: 'Segoe UI', Arial;
        font-size: 12px;
        font-weight: 400;
        background: transparent;
        padding: 3px 10px;
        max-width: 260px;
        qproperty-wordWrap: true;
    }
    QMessageBox QLabel#qt_msgbox_label {
        font-size: 15px;
        font-weight: 700;
        color: #FFFFFF;
        padding-top: 14px;
        padding-left: 4px;
        padding-bottom: 2px;
        min-width: 260px;
        max-width: 380px;
    }
    QMessageBox QLabel#qt_msgbox_informativelabel {
        min-width: 260px;
        max-width: 380px;
    }
    QMessageBox QDialogButtonBox {
        background: transparent;
        padding: 10px 16px 16px 16px;
    }
    QMessageBox QPushButton {
        background-color: #7C3E66;
        color: #F5EDF2;
        border: 1px solid #7C3E66;
        border-radius: 8px;
        padding: 7px 26px;
        font-family: 'Segoe UI', Arial;
        font-size: 12px;
        font-weight: 600;
        min-width: 60px;
        margin: 2px;
    }
    QMessageBox QPushButton:hover {
        background-color: #96517F;
        border: 1px solid #B06B96;
        color: #FFFFFF;
    }
    QMessageBox QPushButton:pressed {
        background-color: #5C2E4D;
        border: 1px solid #5C2E4D;
    }
    QMessageBox QPushButton:focus {
        outline: none;
    }
)";

static void applyWrapAndWidth(QMessageBox &box)
{

    if (QLabel *body = box.findChild<QLabel *>("qt_msgbox_informativelabel")) {
        body->setWordWrap(true);
        body->setMaximumWidth(340);
        body->setMinimumWidth(220);
    }
    if (QLabel *header = box.findChild<QLabel *>("qt_msgbox_label")) {
        header->setWordWrap(true);
        header->setMaximumWidth(340);
    }
}

static void showIconMessage(QWidget *parent, const QString &iconPath, const QString &title,
                             const QString &text, QMessageBox::Icon fallbackIcon)
{
    QMessageBox box(parent);
    box.setText(title);
    box.setInformativeText(text);
    box.setStandardButtons(QMessageBox::Ok);

    QPixmap pix(iconPath);
    if (!pix.isNull())
        box.setIconPixmap(pix.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    else
        box.setIcon(fallbackIcon);

    applyWrapAndWidth(box);

    box.exec();
}

void StyledMessageBox::information(QWidget *parent, const QString &title, const QString &text)
{
    showIconMessage(parent, ":/information icon.png", title, text, QMessageBox::Information);
}

void StyledMessageBox::success(QWidget *parent, const QString &title, const QString &text)
{
    showIconMessage(parent, ":/success icon.png", title, text, QMessageBox::Information);
}

void StyledMessageBox::warning(QWidget *parent, const QString &title, const QString &text)
{
    showIconMessage(parent, ":/warning.png", title, text, QMessageBox::Warning);
}

void StyledMessageBox::error(QWidget *parent, const QString &title, const QString &text)
{
    showIconMessage(parent, ":/error icon.png", title, text, QMessageBox::Critical);
}

bool StyledMessageBox::question(QWidget *parent, const QString &title, const QString &text)
{
    QMessageBox box(parent);
    box.setWindowTitle(title);
    box.setText(text);
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.setDefaultButton(QMessageBox::No);

    QPixmap pix(":/quastion icon.png");
    if (!pix.isNull())
        box.setIconPixmap(pix.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    else
        box.setIcon(QMessageBox::Question);

    return box.exec() == QMessageBox::Yes;
}
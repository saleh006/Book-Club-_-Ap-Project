#ifndef STYLEDMESSAGEBOX_H
#define STYLEDMESSAGEBOX_H

#include <QString>
#include <QWidget>

extern const QString kGlobalMessageBoxStyle;

class StyledMessageBox
{
public:
    static void information(QWidget *parent, const QString &title, const QString &text);
    static void success(QWidget *parent, const QString &title, const QString &text);
    static void warning(QWidget *parent, const QString &title, const QString &text);
    static void error(QWidget *parent, const QString &title, const QString &text);
    static bool question(QWidget *parent, const QString &title, const QString &text);
};

#endif // STYLEDMESSAGEBOX_H
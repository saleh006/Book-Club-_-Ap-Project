#ifndef PDFREADERDIALOG_H
#define PDFREADERDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPdfDocument;
class QPdfView;
class QLabel;
class QSpinBox;
class QToolButton;
QT_END_NAMESPACE

class PdfReaderDialog : public QDialog
{
    Q_OBJECT
public:

    explicit PdfReaderDialog(const QString &pdfPath,const QString &bookTitle,int bookId,
                             int startPage,QWidget *parent = nullptr);

    ~PdfReaderDialog() override;
    bool isDocumentValid() const;

signals:
    void readingProgressChanged(int id, int lastPage, int pageCount);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi(const QString &bookTitle);
    void beginLoad(const QString &pdfPath);
    void onStatusChanged();
    void goToPage(int zeroBasedPage);
    void updateNavControls();
    void applyZoom(qreal factor);
    void showLoadError(const QString &message);

    QPdfDocument *m_document = nullptr;
    QPdfView *m_view = nullptr;

    QLabel *m_titleLabel = nullptr;
    QSpinBox *m_pageSpin = nullptr;
    QLabel *m_pageCountLabel = nullptr;
    QToolButton *m_prevBtn = nullptr;
    QToolButton *m_nextBtn = nullptr;
    QToolButton *m_zoomInBtn = nullptr;
    QToolButton *m_zoomOutBtn = nullptr;
    QToolButton *m_fitWidthBtn = nullptr;

    qreal m_zoomFactor = 1.0;
    bool m_documentValid = false;
    int m_bookId = -1;
    int m_pendingStartPage = 0;
};

#endif // PDFREADERDIALOG_H
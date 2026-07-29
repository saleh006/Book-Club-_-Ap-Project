#ifndef PDFREADERDIALOG_H
#define PDFREADERDIALOG_H

#include <QDialog>
#include <QTcpSocket>

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

    ~PdfReaderDialog() override;
    bool isDocumentValid() const;

    PdfReaderDialog(const QString &pdfPath, const QString &bookTitle,
                    int bookId, int startPage, QWidget *parent = nullptr,
                    QTcpSocket *syncSocket = nullptr, int syncRoomId = -1,
                    int syncUserId = -1, bool isRoomCreator = false);

    void handleServerResponse(const QJsonObject &responseObj);

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

    void sendPageSync(int zeroBasedPage);

    QTcpSocket *m_socket = nullptr;
    int m_roomId = -1;
    int m_syncUserId = -1;
    bool m_isRoomCreator = false;
    bool m_applyingRemotePage = false;
    bool m_roomLeftOrClosed = false;
};

#endif // PDFREADERDIALOG_H
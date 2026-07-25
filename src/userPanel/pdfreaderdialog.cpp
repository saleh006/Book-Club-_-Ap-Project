#include "pdfreaderdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QSpinBox>
#include <QLabel>
#include <QFileInfo>
#include <QCloseEvent>
#include <QMessageBox>
#include <QShortcut>
#include <QKeySequence>
#include <QPdfDocument>
#include <QPdfView>
#include <QPdfPageNavigator>

static QString friendlyDocError(QPdfDocument::Error error)
{
    switch (error) {
    case QPdfDocument::Error::FileNotFound:
        return QObject::tr("The book's PDF file could not be found.");
    case QPdfDocument::Error::InvalidFileFormat:
        return QObject::tr("This file isn't a valid PDF, or it's corrupted.");
    case QPdfDocument::Error::IncorrectPassword:
        return QObject::tr("This PDF is password protected and can't be opened here.");
    case QPdfDocument::Error::UnsupportedSecurityScheme:
        return QObject::tr("This PDF uses a security scheme that isn't supported.");
    default:
        return QObject::tr("The book's PDF file couldn't be opened.");
    }
}

PdfReaderDialog::PdfReaderDialog(const QString &pdfPath, const QString &bookTitle,
                                 int bookId, int startPage, QWidget *parent)
    : QDialog(parent), m_bookId(bookId), m_pendingStartPage(qMax(0, startPage))
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setWindowTitle(bookTitle.isEmpty() ? tr("Book Reader") : bookTitle);
    setAttribute(Qt::WA_DeleteOnClose);

    buildUi(bookTitle);

    if (!QFileInfo::exists(pdfPath)) {
        showLoadError(tr("The book's PDF file could not be found."));
        return;
    }
    beginLoad(pdfPath);
}

PdfReaderDialog::~PdfReaderDialog()
{
    if (m_document)
        disconnect(m_document, &QPdfDocument::statusChanged, this, &PdfReaderDialog::onStatusChanged);

    delete m_view;
    m_view = nullptr;
    delete m_document;
    m_document = nullptr;
}

bool PdfReaderDialog::isDocumentValid() const
{
    return m_documentValid;
}

void PdfReaderDialog::buildUi(const QString &bookTitle)
{
    setStyleSheet("background-color:#07050C;");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *toolbar = new QWidget(this);
    toolbar->setStyleSheet("background-color:#140F1B;border-bottom:1px solid #241A2E;");
    auto *tb = new QHBoxLayout(toolbar);
    tb->setContentsMargins(16, 10, 16, 10);
    tb->setSpacing(8);

    auto makeToolButton = [&](const QString &text, const QString &tip) {
        auto *btn = new QToolButton(toolbar);
        btn->setText(text);
        btn->setToolTip(tip);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
                "QToolButton{background:#0B0810;color:#F5F1F7;border:1px solid #241A2E;"
                "border-radius:6px;padding:6px 12px;font-size:13px;}"
                "QToolButton:hover{background:#3A2C46;}"
                "QToolButton:disabled{color:#665A72;}");
        return btn;
    };

    m_titleLabel = new QLabel(bookTitle, toolbar);
    m_titleLabel->setStyleSheet("color:#F5F1F7;font-size:14px;font-weight:600;background:transparent;border:none;");

    auto *pageLabel = new QLabel(tr("Page"), toolbar);
    pageLabel->setStyleSheet("color:#A79AB0;font-size:13px;background:transparent;border:none;");

    m_pageSpin = new QSpinBox(toolbar);
    m_pageSpin->setFocusPolicy(Qt::NoFocus);
    m_pageSpin->setMinimum(1);
    m_pageSpin->setMaximum(1);
    m_pageSpin->setStyleSheet(
                "QSpinBox{background:#0B0810;color:#F5F1F7;border:1px solid #241A2E;"
                "border-radius:6px;padding:4px 6px;min-width:55px;}");
    connect(m_pageSpin, &QSpinBox::editingFinished, this, [this] { goToPage(m_pageSpin->value() - 1); });

    auto *slashLabel = new QLabel(QStringLiteral("/"), toolbar);
    slashLabel->setStyleSheet("color:#A79AB0;font-size:13px;background:transparent;border:none;");

    m_pageCountLabel = new QLabel(toolbar);
    m_pageCountLabel->setStyleSheet("color:#A79AB0;font-size:13px;background:transparent;border:none;");

    m_zoomOutBtn = makeToolButton(QStringLiteral("\u2212"), tr("Zoom out"));
    m_zoomInBtn = makeToolButton(QStringLiteral("+"), tr("Zoom in"));
    m_fitWidthBtn = makeToolButton(tr("Fit Page"), tr("Fit page to window"));
    connect(m_zoomOutBtn, &QToolButton::clicked, this, [this] { applyZoom(m_zoomFactor - 0.15); });
    connect(m_zoomInBtn, &QToolButton::clicked, this, [this] { applyZoom(m_zoomFactor + 0.15); });
    connect(m_fitWidthBtn, &QToolButton::clicked, this, [this] {
        if (m_view) m_view->setZoomMode(QPdfView::ZoomMode::FitInView);
    });

    tb->addWidget(m_titleLabel);
    tb->addStretch();
    tb->addWidget(pageLabel);
    tb->addWidget(m_pageSpin);
    tb->addWidget(slashLabel);
    tb->addWidget(m_pageCountLabel);
    tb->addSpacing(18);
    tb->addWidget(m_zoomOutBtn);
    tb->addWidget(m_fitWidthBtn);
    tb->addWidget(m_zoomInBtn);

    tb->addSpacing(18);

    auto *closeBtn = new QToolButton(toolbar);
    closeBtn->setText(QStringLiteral("Close ✕"));
    closeBtn->setToolTip(tr("Close Reader"));
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(QString(
        "QToolButton{background:#E4577A;color:#FFFFFF;border:none;"
        "border-radius:6px;padding:6px 12px;font-size:13px;font-weight:bold;}"
        "QToolButton:hover{background:#D13D60;}"));

    connect(closeBtn, &QToolButton::clicked, this, &PdfReaderDialog::close);

    tb->addWidget(closeBtn);
    root->addWidget(toolbar);

    m_document = new QPdfDocument(this);
    m_view = new QPdfView(this);
    m_view->setDocument(m_document);
    m_view->setPageMode(QPdfView::PageMode::SinglePage);
    m_view->setZoomMode(QPdfView::ZoomMode::FitInView);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setStyleSheet("background-color:#07050C;border:none;");

    auto *viewerRow = new QWidget(this);
    viewerRow->setStyleSheet("background-color:#07050C;");
    auto *viewerLayout = new QHBoxLayout(viewerRow);
    viewerLayout->setContentsMargins(14, 14, 14, 14);
    viewerLayout->setSpacing(14);

    auto makeNavButton = [&](const QString &text, const QString &tip) {
        auto *btn = new QToolButton(viewerRow);
        btn->setText(text);
        btn->setToolTip(tip);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedSize(52, 128);
        btn->setStyleSheet(
                "QToolButton{background:rgba(20,15,27,200);color:#F5F1F7;"
                "border:1px solid #241A2E;border-radius:26px;"
                "font-size:22px;font-weight:600;}"
                "QToolButton:hover{background:#0B0810;border-color:#3A2C46;}"
                "QToolButton:disabled{color:#4A4052;background:rgba(20,15,27,110);}");
        return btn;
    };

    m_prevBtn = makeNavButton(QStringLiteral("\u25C0"), tr("Previous page"));
    m_nextBtn = makeNavButton(QStringLiteral("\u25B6"), tr("Next page"));
    connect(m_prevBtn, &QToolButton::clicked, this, [this] { goToPage(m_pageSpin->value() - 2); });
    connect(m_nextBtn, &QToolButton::clicked, this, [this] { goToPage(m_pageSpin->value()); });

    viewerLayout->addWidget(m_prevBtn, 0, Qt::AlignVCenter);
    viewerLayout->addWidget(m_view, 1);
    viewerLayout->addWidget(m_nextBtn, 0, Qt::AlignVCenter);

    root->addWidget(viewerRow, 1);

    auto *prevShortcut = new QShortcut(QKeySequence(Qt::Key_Left), this);
    prevShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(prevShortcut, &QShortcut::activated, this, [this] {
        if (m_documentValid) goToPage(m_view->pageNavigator()->currentPage() - 1);
    });
    auto *nextShortcut = new QShortcut(QKeySequence(Qt::Key_Right), this);
    nextShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(nextShortcut, &QShortcut::activated, this, [this] {
        if (m_documentValid) goToPage(m_view->pageNavigator()->currentPage() + 1);
    });

    for (auto *b : {m_prevBtn, m_nextBtn, m_zoomInBtn, m_zoomOutBtn, m_fitWidthBtn})
        b->setEnabled(false);
    m_pageSpin->setEnabled(false);
    m_pageCountLabel->setText(tr("\u2014"));

    auto *escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escShortcut, &QShortcut::activated, this, &PdfReaderDialog::close);

    this->setFocus();
}

void PdfReaderDialog::beginLoad(const QString &pdfPath)
{
    connect(m_document, &QPdfDocument::statusChanged, this, &PdfReaderDialog::onStatusChanged);
    m_document->load(pdfPath);
}

void PdfReaderDialog::onStatusChanged()
{
    const QPdfDocument::Status status = m_document->status();
    if (status == QPdfDocument::Status::Loading)
        return;

    if (status != QPdfDocument::Status::Ready) {
        showLoadError(friendlyDocError(m_document->error()));
        return;
    }

    m_documentValid = true;
    const int pageCount = m_document->pageCount();
    m_pageSpin->setMaximum(qMax(1, pageCount));
    m_pageCountLabel->setText(QString::number(pageCount));

    connect(m_view->pageNavigator(), &QPdfPageNavigator::currentPageChanged,this, [this](int page)
    {
        m_pageSpin->blockSignals(true);
        m_pageSpin->setValue(page + 1);
        m_pageSpin->blockSignals(false);
        updateNavControls();
    });

    for (auto *b : {m_prevBtn, m_nextBtn, m_zoomInBtn, m_zoomOutBtn, m_fitWidthBtn})
        b->setEnabled(true);
    m_pageSpin->setEnabled(true);

    int startPage = m_pendingStartPage;
    if (startPage < 0 || startPage >= pageCount)
        startPage = 0;
    goToPage(startPage);
    updateNavControls();
}

void PdfReaderDialog::goToPage(int zeroBasedPage)
{
    if (!m_documentValid || !m_document || m_document->pageCount() <= 0)
        return;
    zeroBasedPage = qBound(0, zeroBasedPage, m_document->pageCount() - 1);
    m_view->pageNavigator()->jump(zeroBasedPage, QPointF());
}

void PdfReaderDialog::updateNavControls()
{
    if (!m_documentValid) return;
    const int page = m_view->pageNavigator()->currentPage();
    const int count = m_document->pageCount();
    m_prevBtn->setEnabled(page > 0);
    m_nextBtn->setEnabled(page < count - 1);
}

void PdfReaderDialog::applyZoom(qreal factor)
{
    m_zoomFactor = qBound(0.25, factor, 4.0);
    m_view->setZoomMode(QPdfView::ZoomMode::Custom);
    m_view->setZoomFactor(m_zoomFactor);
}

void PdfReaderDialog::showLoadError(const QString &message)
{
    m_documentValid = false;
    m_pageCountLabel->setText(tr("\u2014"));
    QMessageBox::warning(this, tr("Unable to open book"), message);
}

void PdfReaderDialog::closeEvent(QCloseEvent *event)
{
    if (m_documentValid && m_view && m_view->pageNavigator())
        emit readingProgressChanged(m_bookId, m_view->pageNavigator()->currentPage());
    QDialog::closeEvent(event);
}
#include "addeditbookdialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include "QMessageBox"



AddEditBookDialog::AddEditBookDialog(const Book &existingBook, QWidget *parent)
    : QDialog(parent), m_book(existingBook)
{
    setupUi(existingBook.id > 0);
}

void AddEditBookDialog::setupUi(bool isEditMode)
{
    setWindowTitle(isEditMode ? "Edit Book" : "Add Book");
    resize(550, 520);
    setStyleSheet(R"(
        QDialog {
            background-color: #120E14;
            color: #EAEAEA;
        }

        QLabel {
            color: #A594B3;
            background-color: #120E14;
            border: 1px solid #5F2E4F;
            border-radius: 2px;
            padding: 6px 10px;
            min-width: 90px;
            qproperty-alignment: 'Qt::AlignCenter';
        }

        QLineEdit {
            background-color: #1F1724;
            border: 1px solid #5F2E4F;
            border-radius: 12px;
            padding: 6px 12px;
            color: #EAEAEA;
        }

        QTextEdit {
            background-color: #1F1724;
            border: 1px solid #5F2E4F;
            border-radius: 12px;
            padding: 8px;
            color: #EAEAEA;
        }

        QLineEdit:focus,
        QTextEdit:focus {
            border: 1px solid #7C3E66;
        }

        QDoubleSpinBox {
            background-color: #1F1724;
            border: 1px solid #5F2E4F;
            border-radius: 12px;
            color: #EAEAEA;
            min-height: 32px;
            padding-left: 10px;
            padding-right: 25px;
        }

        QDoubleSpinBox:focus {
            border: 1px solid #7C3E66;
        }

        QPushButton {
            background-color: #7C3E66;
            border: none;
            border-radius: 12px;
            padding: 8px 20px;
            color: white;
            font-weight: bold;
        }

        QPushButton:hover {
            background-color: #5F2E4F;
        }
    )");

    m_titleEdit = new QLineEdit(m_book.title, this);
    m_authorEdit = new QLineEdit(m_book.author, this);
    m_genreEdit = new QLineEdit(m_book.genre, this);

    m_descriptionEdit = new QTextEdit(m_book.description, this);
    m_descriptionEdit->setFixedHeight(100);

    m_priceSpin = new QDoubleSpinBox(this);
    m_priceSpin->setRange(0.0, 999999.0);
    m_priceSpin->setDecimals(2);
    m_priceSpin->setPrefix("$ ");
    m_priceSpin->setValue(m_book.price);
    m_priceSpin->setMinimumHeight(35);

    m_coverPathEdit = new QLineEdit(m_book.coverImagePath, this);
    m_pdfPathEdit = new QLineEdit(m_book.pdfPath, this);

    QPushButton *browseCoverBtn = new QPushButton("Browse...", this);
    QPushButton *browsePdfBtn = new QPushButton("Browse...", this);

    QString browseStyle = R"(
        QPushButton {
            background-color: #1F1724;
            border: 1px solid #5F2E4F;
            border-radius: 12px;
            padding: 6px 15px;
            color: #EAEAEA;
        }
        QPushButton:hover {
            background-color: #7C3E66;
        }
    )";

    browseCoverBtn->setStyleSheet(browseStyle);
    browsePdfBtn->setStyleSheet(browseStyle);

    QHBoxLayout *coverRow = new QHBoxLayout;
    coverRow->addWidget(m_coverPathEdit);
    coverRow->addWidget(browseCoverBtn);

    QHBoxLayout *pdfRow = new QHBoxLayout;
    pdfRow->addWidget(m_pdfPathEdit);
    pdfRow->addWidget(browsePdfBtn);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    formLayout->setLabelAlignment(Qt::AlignCenter);

    formLayout->setHorizontalSpacing(15);
    formLayout->setVerticalSpacing(14);

    formLayout->addRow("Title", m_titleEdit);
    formLayout->addRow("Price", m_priceSpin);
    formLayout->addRow("Author", m_authorEdit);
    formLayout->addRow("Genre", m_genreEdit);
    formLayout->addRow("Description", m_descriptionEdit);
    formLayout->addRow("Cover Path", coverRow);
    formLayout->addRow("PDF Path", pdfRow);

    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
            Qt::Horizontal,
            this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);
}

static bool uploadFileToServer(const QString &localFilePath, const QString &fileType,
                               QString &outServerPath, QString &errorMsg)
{
    QFile file(localFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        errorMsg = "Could not open file: " + localFilePath;
        return false;
    }
    QByteArray fileBytes = file.readAll();
    file.close();

    QFileInfo info(localFilePath);

    QJsonObject req;
    req["action"] = "upload_file";
    req["fileType"] = fileType;
    req["fileName"] = info.fileName();
    req["fileData"] = QString(fileBytes.toBase64());

    QTcpSocket socket;
    socket.connectToHost("127.0.0.1", 1234);
    if (!socket.waitForConnected(3000)) {
        errorMsg = "Could not connect to server for upload.";
        return false;
    }

    socket.write(QJsonDocument(req).toJson(QJsonDocument::Compact) + "\n");

    if (!socket.waitForReadyRead(10000)) {
        errorMsg = "Server did not respond to upload.";
        socket.disconnectFromHost();
        return false;
    }

    QByteArray responseData = socket.readAll();
    while (socket.waitForReadyRead(500)) {
        responseData += socket.readAll(); // in case the response arrives in multiple chunks
    }
    socket.disconnectFromHost();

    QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
    if (!responseDoc.isObject()) {
        errorMsg = "Invalid upload response.";
        return false;
    }
    QJsonObject responseObj = responseDoc.object();
    if (responseObj["type"].toString() != "upload_result" || !responseObj["success"].toBool()) {
        errorMsg = responseObj["message"].toString();
        return false;
    }
    outServerPath = responseObj["serverPath"].toString();
    return true;
}

Book AddEditBookDialog::resultBook() const
{
    Book b = m_book; // preserves id, publisherId if editing
    b.title = m_titleEdit->text().trimmed();
    b.author = m_authorEdit->text().trimmed();
    b.genre = m_genreEdit->text().trimmed();
    b.description = m_descriptionEdit->toPlainText().trimmed();
    b.price = m_priceSpin->value();
    b.coverImagePath = m_coverPathEdit->text().trimmed();
    b.pdfPath = m_pdfPathEdit->text().trimmed();
    return b;
}


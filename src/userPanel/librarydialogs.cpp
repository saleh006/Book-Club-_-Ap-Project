#include "librarydialogs.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>

namespace {
const char *kDialogChrome =
    "QDialog{background-color:#181320;border-radius:18px;font-family:'Segoe UI';}"
    "QLabel{color:#EAEAEA;background:transparent;border:none;}"
    "QLineEdit,QPlainTextEdit{background-color:#231B2F;border:1px solid rgba(255,255,255,20);"
    "border-radius:10px;padding:8px 12px;color:#EAEAEA;font-size:13px;}"
    "QLineEdit:focus,QPlainTextEdit:focus{border-color:#A855F7;}";

QPushButton *makeCloseButton(QWidget *parent)
{
    auto *btn = new QPushButton(QStringLiteral("\u2715"), parent);
    btn->setFixedSize(26, 26);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(
        "QPushButton{background:transparent;border:none;color:rgba(255,255,255,140);font-size:13px;}"
        "QPushButton:hover{color:#FFFFFF;}");
    return btn;
}
} // namespace

// --------------------------------------------------------- CreateShelfDialog

CreateShelfDialog::CreateShelfDialog(QWidget *parent, bool editMode,
                                     const QString &existingName,
                                     const QString &existingDescription,
                                     const QColor &existingColor)
    : QDialog(parent), m_selectedColor(existingColor)
{
    setWindowTitle(editMode ? "Edit Shelf" : "Create New Shelf");
    setModal(true);
    setFixedWidth(420);
    setStyleSheet(kDialogChrome);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(14);

    auto *headRow = new QHBoxLayout;
    auto *titleLbl = new QLabel(editMode ? "Edit Shelf" : "Create New Shelf", this);
    titleLbl->setStyleSheet("font-size:16px; font-weight:700; color:#FFFFFF;");
    auto *closeBtn = makeCloseButton(this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    headRow->addWidget(titleLbl);
    headRow->addStretch();
    headRow->addWidget(closeBtn);
    root->addLayout(headRow);

    auto *nameLbl = new QLabel("Shelf Name", this);
    nameLbl->setStyleSheet("font-size:12px; color: rgba(255,255,255,150); font-weight:600;");
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Enter shelf name");
    m_nameEdit->setFixedHeight(38);
    m_nameEdit->setText(existingName);
    root->addWidget(nameLbl);
    root->addWidget(m_nameEdit);

    auto *descLbl = new QLabel("Description (Optional)", this);
    descLbl->setStyleSheet("font-size:12px; color: rgba(255,255,255,150); font-weight:600;");
    m_descEdit = new QPlainTextEdit(this);
    m_descEdit->setPlaceholderText("Enter description...");
    m_descEdit->setFixedHeight(64);
    m_descEdit->setPlainText(existingDescription);
    root->addWidget(descLbl);
    root->addWidget(m_descEdit);

    auto *colorLbl = new QLabel("Choose Color", this);
    colorLbl->setStyleSheet("font-size:12px; color: rgba(255,255,255,150); font-weight:600;");
    root->addWidget(colorLbl);

    auto *swatchRow = new QHBoxLayout;
    swatchRow->setSpacing(10);
    m_colorGroup = new QButtonGroup(this);
    static const QVector<QColor> palette = {
        QColor("#A855F7"), QColor("#3B82F6"), QColor("#10B981"), QColor("#F59E0B"),
        QColor("#EF4444"), QColor("#EC4899"), QColor("#94A3B8")
    };
    for (const QColor &c : palette) {
        QPushButton *swatch = makeSwatch(c);
        m_colorGroup->addButton(swatch);
        swatchRow->addWidget(swatch);
        if (c.name().compare(existingColor.name(), Qt::CaseInsensitive) == 0)
            swatch->setChecked(true);
    }
    if (m_colorGroup->checkedButton() == nullptr && !m_colorGroup->buttons().isEmpty())
        m_colorGroup->buttons().first()->setChecked(true);
    swatchRow->addStretch();
    root->addLayout(swatchRow);

    connect(m_colorGroup, &QButtonGroup::buttonClicked, this, [this](QAbstractButton *btn) {
        m_selectedColor = QColor(btn->property("swatchColor").toString());
    });

    root->addSpacing(6);

    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);

    auto *cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setFixedHeight(42);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet(
        "QPushButton{background:transparent;border:1px solid rgba(255,255,255,30);"
        "border-radius:10px;color:#EAEAEA;font-size:13px;}"
        "QPushButton:hover{background-color:#231B2F;}");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    auto *createBtn = new QPushButton(editMode ? "Save Changes" : "Create Shelf", this);
    createBtn->setFixedHeight(42);
    createBtn->setCursor(Qt::PointingHandCursor);
    createBtn->setStyleSheet(
        "QPushButton{background-color:#A855F7;border:none;border-radius:10px;"
        "color:white;font-size:13px;font-weight:700;}"
        "QPushButton:hover{background-color:#C084FC;}");
    connect(createBtn, &QPushButton::clicked, this, [this] {
        if (m_nameEdit->text().trimmed().isEmpty()) {
            m_nameEdit->setPlaceholderText("Shelf name is required");
            m_nameEdit->setFocus();
            return;
        }
        accept();
    });

    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(createBtn);
    root->addLayout(btnRow);
}

QPushButton *CreateShelfDialog::makeSwatch(const QColor &color)
{
    auto *btn = new QPushButton(this);
    btn->setCheckable(true);
    btn->setFixedSize(30, 30);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setProperty("swatchColor", color.name());
    btn->setStyleSheet(QString(
                           "QPushButton{background-color:%1;border-radius:15px;border:2px solid transparent;}"
                           "QPushButton:checked{border:2px solid white;}").arg(color.name()));
    return btn;
}

QString CreateShelfDialog::shelfName() const { return m_nameEdit->text().trimmed(); }
QString CreateShelfDialog::shelfDescription() const { return m_descEdit->toPlainText().trimmed(); }

// --------------------------------------------------------- MoveToShelfDialog

MoveToShelfDialog::MoveToShelfDialog(const QVector<ShelfSummary> &shelves,
                                     int currentShelfId, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Move to Shelf");
    setModal(true);
    setFixedWidth(360);
    setStyleSheet(kDialogChrome);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(22, 20, 22, 20);
    root->setSpacing(10);

    auto *headRow = new QHBoxLayout;
    auto *titleLbl = new QLabel("Move to Shelf", this);
    titleLbl->setStyleSheet("font-size:16px; font-weight:700; color:#FFFFFF;");
    auto *closeBtn = makeCloseButton(this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    headRow->addWidget(titleLbl);
    headRow->addStretch();
    headRow->addWidget(closeBtn);
    root->addLayout(headRow);

    m_group = new QButtonGroup(this);
    for (const ShelfSummary &s : shelves) {
        auto *row = new QPushButton(this);
        row->setCheckable(true);
        row->setFixedHeight(42);
        row->setCursor(Qt::PointingHandCursor);
        row->setText(QString("  %1   %2").arg(shelfIconForTitle(s.shelf.title), s.shelf.title));
        row->setStyleSheet(
            "QPushButton{background-color:#231B2F;border:1px solid rgba(255,255,255,20);"
            "border-radius:10px;color:#EAEAEA;font-size:13px;text-align:left;padding-left:12px;}"
            "QPushButton:checked{background-color:#A855F7;color:white;border-color:#C084FC;}"
            "QPushButton:hover{border-color:#A855F7;}");
        row->setProperty("shelfId", s.shelf.id);
        m_group->addButton(row);
        root->addWidget(row);
        if (s.shelf.id == currentShelfId) {
            row->setChecked(true);
            m_selectedShelfId = s.shelf.id;
        }
    }
    if (m_selectedShelfId == -1 && !m_group->buttons().isEmpty()) {
        m_group->buttons().first()->setChecked(true);
        m_selectedShelfId = m_group->buttons().first()->property("shelfId").toInt();
    }
    connect(m_group, &QButtonGroup::buttonClicked, this, [this](QAbstractButton *btn) {
        m_selectedShelfId = btn->property("shelfId").toInt();
    });

    root->addSpacing(4);
    auto *createNewBtn = new QPushButton("+ Create New Shelf", this);
    createNewBtn->setCursor(Qt::PointingHandCursor);
    createNewBtn->setStyleSheet(
        "QPushButton{background:transparent;border:1px dashed rgba(255,255,255,40);"
        "border-radius:10px;color:#A855F7;font-size:12px;font-weight:600;padding:8px;}"
        "QPushButton:hover{border-color:#A855F7;background-color:#231B2F;}");
    connect(createNewBtn, &QPushButton::clicked, this, [this] {
        emit createNewShelfRequested();
        reject();
    });
    root->addWidget(createNewBtn);

    root->addSpacing(4);
    auto *moveBtn = new QPushButton("Move", this);
    moveBtn->setFixedHeight(42);
    moveBtn->setCursor(Qt::PointingHandCursor);
    moveBtn->setStyleSheet(
        "QPushButton{background-color:#A855F7;border:none;border-radius:10px;"
        "color:white;font-size:13px;font-weight:700;}"
        "QPushButton:hover{background-color:#C084FC;}");
    connect(moveBtn, &QPushButton::clicked, this, [this] {
        if (m_selectedShelfId != -1) accept();
    });
    root->addWidget(moveBtn);
}

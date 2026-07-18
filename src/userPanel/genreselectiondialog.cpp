#include "genreselectiondialog.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPainter>
#include <QLinearGradient>

namespace {

struct GenreEntry { const char *image; const char *name; };

const GenreEntry kGenres[] = {
    {":/genres/scifi.jpg",      "Science Fiction"},
    {":/genres/fantasy.jpg",    "Fantasy"},
    {":/genres/mystery.jpg",    "Mystery"},
    {":/genres/romance.jpg",    "Romance"},
    {":/genres/horror.jpg",     "Horror"},
    {":/genres/history.jpg",    "History"},
    {":/genres/technology.jpg", "Technology"},
    {":/genres/business.jpg",   "Business"},
    {":/genres/psychology.jpg", "Psychology"},
    {":/genres/art.jpg",        "Art & Design"},
    {":/genres/cooking.jpg",    "Cooking"},
    {":/genres/health.jpg",     "Health & Sport"},
    {":/genres/children.jpg",   "Children"},
    {":/genres/poetry.jpg",     "Poetry"},
    {":/genres/travel.jpg",     "Travel"},
    {":/genres/biography.jpg",  "Biography"},
    };

constexpr int kMinSelection = 3;
constexpr int kColumns = 4;
const QSize kCardSize(118, 84);

// Renders one genre card: image (center-cropped), dark gradient, name,
// and — when selected — a purple tint plus a checkmark badge.
QIcon makeGenreCard(const QString &imagePath, const QString &name,
                    const QSize &size, bool selected)
{
    QPixmap canvas(size);
    canvas.fill(QColor("#120E14"));   // visible fallback if the image is missing

    QPainter p(&canvas);
    p.setRenderHint(QPainter::Antialiasing);

    QPixmap img(imagePath);
    if (!img.isNull()) {
        img = img.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        p.drawPixmap((size.width()  - img.width())  / 2,
                     (size.height() - img.height()) / 2, img);
    }

    QLinearGradient g(0, size.height() * 0.45, 0, size.height());
    g.setColorAt(0, QColor(0, 0, 0, 0));
    g.setColorAt(1, QColor(0, 0, 0, 200));
    p.fillRect(canvas.rect(), g);

    if (selected)
        p.fillRect(canvas.rect(), QColor(124, 62, 102, 90));
    else
        p.fillRect(canvas.rect(), QColor(0, 0, 0, 60));

    p.setPen(Qt::white);
    QFont f = p.font();
    f.setPointSize(9);
    f.setBold(true);
    p.setFont(f);
    p.drawText(canvas.rect().adjusted(6, 0, -6, -8),
               Qt::AlignBottom | Qt::AlignHCenter, name);

    if (selected) {
        p.setBrush(QColor("#7C3E66"));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPoint(size.width() - 16, 16), 11, 11);
        p.setPen(Qt::white);
        p.drawText(QRect(size.width() - 27, 5, 22, 22), Qt::AlignCenter, "✓");
    }

    p.end();
    return QIcon(canvas);
}

} // namespace

GenreSelectionDialog::GenreSelectionDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Choose Your Interests");
    setFixedSize(560, 560);
    setStyleSheet("QDialog { background-color: #060508; }");
    // first-run step — user must pick, no X-button escape
    setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(30, 25, 30, 25);
    layout->setSpacing(8);

    auto *title = new QLabel("What do you like to read? 📚", this);
    title->setStyleSheet("color: #FFFFFF; font-size: 20px; font-weight: bold; background: transparent;");
    auto *subtitle = new QLabel(
        QString("Pick at least %1 genres — we'll use them to recommend books you'll love.")
            .arg(kMinSelection), this);
    subtitle->setStyleSheet("color: #9A8FA0; font-size: 12px; background: transparent;");
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(12);

    const QString cardStyle =
        "QPushButton { background: transparent; border: 2px solid #1F1724; border-radius: 10px; padding: 0; }"
        "QPushButton:hover { border-color: #7C3E66; }"
        "QPushButton:checked { border: 2px solid #B06B96; }";

    auto *grid = new QGridLayout;
    grid->setSpacing(10);
    int i = 0;
    for (const auto &entry : kGenres) {
        auto *btn = new QPushButton(this);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedSize(kCardSize);
        btn->setIconSize(kCardSize);
        btn->setStyleSheet(cardStyle);
        btn->setProperty("genreName", entry.name);
        btn->setIcon(makeGenreCard(entry.image, entry.name, kCardSize, false));

        const QString img = entry.image;
        const QString name = entry.name;
        connect(btn, &QPushButton::toggled, this, [this, btn, img, name](bool on) {
            btn->setIcon(makeGenreCard(img, name, kCardSize, on));
            updateContinueButton();
        });

        grid->addWidget(btn, i / kColumns, i % kColumns);
        m_genreButtons.push_back(btn);
        ++i;
    }
    layout->addLayout(grid);
    layout->addStretch();

    auto *bottomRow = new QHBoxLayout;
    m_counterLabel = new QLabel(QString("0 / %1 selected").arg(kMinSelection), this);
    m_counterLabel->setStyleSheet("color: #9A8FA0; font-size: 12px; background: transparent;");
    m_continueBtn = new QPushButton("Continue  →", this);
    m_continueBtn->setEnabled(false);
    m_continueBtn->setCursor(Qt::PointingHandCursor);
    m_continueBtn->setStyleSheet(
        "QPushButton { background-color: #7C3E66; border: none; border-radius: 8px;"
        "  padding: 10px 24px; color: white; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #B06B96; }"
        "QPushButton:disabled { background-color: #1F1724; color: #5A5060; }");
    connect(m_continueBtn, &QPushButton::clicked, this, &QDialog::accept);

    bottomRow->addWidget(m_counterLabel);
    bottomRow->addStretch();
    bottomRow->addWidget(m_continueBtn);
    layout->addLayout(bottomRow);
}

QStringList GenreSelectionDialog::selectedGenres() const
{
    QStringList out;
    for (const auto *btn : m_genreButtons)
        if (btn->isChecked())
            out << btn->property("genreName").toString();
    return out;
}

void GenreSelectionDialog::updateContinueButton()
{
    const int n = selectedGenres().size();
    m_counterLabel->setText(n >= kMinSelection
                                ? QString("%1 selected ✓").arg(n)
                                : QString("%1 / %2 selected").arg(n).arg(kMinSelection));
    m_continueBtn->setEnabled(n >= kMinSelection);
}
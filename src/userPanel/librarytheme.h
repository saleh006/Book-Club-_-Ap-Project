#ifndef LIBRARYTHEME_H
#define LIBRARYTHEME_H

#include <QColor>

// Shared visual constants for the "My Library" feature (dark-purple theme,
// 18px card radius, Segoe UI). Centralised so every widget in this feature
// stays in sync if the palette ever needs to change.
namespace LibraryTheme {
inline const QColor kBackground    = QColor("#09070D");
inline const QColor kCard          = QColor("#181320");
inline const QColor kCardSecondary = QColor("#231B2F");
inline const QColor kAccent        = QColor("#A855F7");
inline const QColor kAccentHover   = QColor("#C084FC");
inline const QColor kBorder        = QColor(255, 255, 255, 20);  // rgba(255,255,255,0.08)
inline const QColor kTextPrimary   = QColor("#FFFFFF");
inline const QColor kTextDim       = QColor(255, 255, 255, 150);
inline const int    kRadius        = 18;
inline const char  *kFontFamily    = "Segoe UI";
}

#endif // LIBRARYTHEME_H

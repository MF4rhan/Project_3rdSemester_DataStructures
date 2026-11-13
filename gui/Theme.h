#pragma once

#include <QColor>
#include <QIcon>
#include <QString>

enum class ThemePreset
{
    Graphite = 0,
    Midnight = 1,
    Evergreen = 2
};

struct ThemePalette
{
    ThemePreset preset = ThemePreset::Graphite;
    QString name;
    QString key;

    QString windowBg;
    QString surfaceBg;
    QString surfaceAltBg;
    QString hoverBg;
    QString border;
    QString borderStrong;

    QString textPrimary;
    QString textSecondary;
    QString textMuted;

    QString accent;
    QString accentHover;
    QString accentPressed;
    QString accentTint;
    QString accentText;

    QString success;
    QString successTint;
    QString warning;
    QString warningTint;
    QString error;
    QString errorTint;
};

ThemePalette themePalette(ThemePreset preset);
ThemePreset themePresetFromKey(const QString& key);
ThemePreset loadSavedTheme();
void saveTheme(ThemePreset preset);

QString buildApplicationStyleSheet(const ThemePalette& palette);

QIcon createAppLogoIcon(const ThemePalette& palette);
QIcon createUploadIcon(const QColor& color);
QIcon createFileIcon(const QColor& color);
QIcon createFolderIcon(const QColor& color);
QIcon createCloseIcon(const QColor& color);
QIcon createChevronIcon(const QColor& color, bool expanded);

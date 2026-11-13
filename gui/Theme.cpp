#include "Theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QSettings>

namespace {
    QPixmap makeCanvas(int logicalSize, qreal dpr)
    {
        const int px = static_cast<int>(logicalSize * dpr);
        QPixmap pixmap(px, px);
        pixmap.setDevicePixelRatio(dpr);
        pixmap.fill(Qt::transparent);
        return pixmap;
    }
}

ThemePalette themePalette(ThemePreset preset)
{
    switch (preset)
    {
    case ThemePreset::Midnight:
        return ThemePalette{
            ThemePreset::Midnight,
            QStringLiteral("Midnight"),
            QStringLiteral("midnight"),
            QStringLiteral("#131820"),
            QStringLiteral("#1A212C"),
            QStringLiteral("#202936"),
            QStringLiteral("#273242"),
            QStringLiteral("#2B3749"),
            QStringLiteral("#3E4F68"),
            QStringLiteral("#E5ECF4"),
            QStringLiteral("#97A6BA"),
            QStringLiteral("#68778C"),
            QStringLiteral("#38A3B8"),
            QStringLiteral("#46B2C7"),
            QStringLiteral("#2D8FA2"),
            QStringLiteral("rgba(56, 163, 184, 0.14)"),
            QStringLiteral("#F4F8FB"),
            QStringLiteral("#41B883"),
            QStringLiteral("rgba(65, 184, 131, 0.14)"),
            QStringLiteral("#D99B4E"),
            QStringLiteral("rgba(217, 155, 78, 0.14)"),
            QStringLiteral("#E06C75"),
            QStringLiteral("rgba(224, 108, 117, 0.13)")
        };

    case ThemePreset::Evergreen:
        return ThemePalette{
            ThemePreset::Evergreen,
            QStringLiteral("Evergreen"),
            QStringLiteral("evergreen"),
            QStringLiteral("#151B19"),
            QStringLiteral("#1C2421"),
            QStringLiteral("#222C28"),
            QStringLiteral("#2A3631"),
            QStringLiteral("#2E3B36"),
            QStringLiteral("#42544D"),
            QStringLiteral("#E6ECE9"),
            QStringLiteral("#9AA9A2"),
            QStringLiteral("#6B7A73"),
            QStringLiteral("#2FA889"),
            QStringLiteral("#3BB898"),
            QStringLiteral("#268F74"),
            QStringLiteral("rgba(47, 168, 137, 0.14)"),
            QStringLiteral("#F3FBF8"),
            QStringLiteral("#4CC38A"),
            QStringLiteral("rgba(76, 195, 138, 0.14)"),
            QStringLiteral("#D99B4E"),
            QStringLiteral("rgba(217, 155, 78, 0.14)"),
            QStringLiteral("#E06C75"),
            QStringLiteral("rgba(224, 108, 117, 0.13)")
        };

    case ThemePreset::Graphite:
    default:
        return ThemePalette{
            ThemePreset::Graphite,
            QStringLiteral("Graphite"),
            QStringLiteral("graphite"),
            QStringLiteral("#18191D"),
            QStringLiteral("#202228"),
            QStringLiteral("#262930"),
            QStringLiteral("#2D313A"),
            QStringLiteral("#30343E"),
            QStringLiteral("#454B59"),
            QStringLiteral("#E8EAEF"),
            QStringLiteral("#A0A6B4"),
            QStringLiteral("#707684"),
            QStringLiteral("#6366F1"),
            QStringLiteral("#7376F5"),
            QStringLiteral("#5255DC"),
            QStringLiteral("rgba(99, 102, 241, 0.15)"),
            QStringLiteral("#F4F5FF"),
            QStringLiteral("#3FB984"),
            QStringLiteral("rgba(63, 185, 132, 0.14)"),
            QStringLiteral("#D99B4E"),
            QStringLiteral("rgba(217, 155, 78, 0.14)"),
            QStringLiteral("#E06C75"),
            QStringLiteral("rgba(224, 108, 117, 0.13)")
        };
    }
}

ThemePreset themePresetFromKey(const QString& key)
{
    if (key == QLatin1String("midnight"))
        return ThemePreset::Midnight;
    if (key == QLatin1String("evergreen"))
        return ThemePreset::Evergreen;
    return ThemePreset::Graphite;
}

ThemePreset loadSavedTheme()
{
    QSettings settings(QStringLiteral("HuffmanProject"), QStringLiteral("HuffmanStudio"));
    return themePresetFromKey(settings.value(QStringLiteral("appearance/theme"), QStringLiteral("graphite")).toString());
}

void saveTheme(ThemePreset preset)
{
    QSettings settings(QStringLiteral("HuffmanProject"), QStringLiteral("HuffmanStudio"));
    settings.setValue(QStringLiteral("appearance/theme"), themePalette(preset).key);
}

QString buildApplicationStyleSheet(const ThemePalette& p)
{
    QString css = QStringLiteral(R"(
        QMainWindow, QDialog {
            background-color: %WINDOW_BG%;
            color: %TEXT_PRIMARY%;
        }

        QWidget {
            color: %TEXT_PRIMARY%;
            font-size: 13px;
        }

        QScrollArea#MainScrollArea, QWidget#ScrollContent {
            background-color: transparent;
            border: none;
        }

        QFrame#HeaderBar {
            background-color: %SURFACE_BG%;
            border-bottom: 1px solid %BORDER%;
        }

        QLabel#AppTitle {
            font-size: 16px;
            font-weight: 600;
            color: %TEXT_PRIMARY%;
        }

        QLabel#AppSubtitle {
            font-size: 12px;
            color: %TEXT_MUTED%;
        }

        QLabel#SectionLabel {
            font-size: 12px;
            font-weight: 600;
            color: %TEXT_SECONDARY%;
        }

        QLabel#MutedLabel, QLabel#FieldLabel {
            font-size: 12px;
            color: %TEXT_SECONDARY%;
        }

        QLabel#HintLabel {
            font-size: 12px;
            color: %TEXT_MUTED%;
        }

        QLabel#WarningHint {
            font-size: 12px;
            color: %WARNING%;
        }

        QLabel#ErrorHint {
            font-size: 12px;
            color: %ERROR%;
        }

        QFrame#SegmentedContainer {
            background-color: %SURFACE_BG%;
            border: 1px solid %BORDER%;
            border-radius: 8px;
        }

        QPushButton#SegmentButton {
            background-color: transparent;
            color: %TEXT_SECONDARY%;
            border: none;
            border-radius: 6px;
            padding: 7px 14px;
            font-weight: 500;
        }

        QPushButton#SegmentButton:hover:!checked:enabled {
            color: %TEXT_PRIMARY%;
            background-color: %HOVER_BG%;
        }

        QPushButton#SegmentButton:checked {
            background-color: %ACCENT%;
            color: %ACCENT_TEXT%;
            font-weight: 600;
        }

        QPushButton#SegmentButton:disabled {
            color: %TEXT_MUTED%;
        }

        QFrame#DropZone {
            background-color: %SURFACE_BG%;
            border: 1px dashed %BORDER_STRONG%;
            border-radius: 9px;
        }

        QFrame#DropZone:hover {
            border-color: %ACCENT%;
            background-color: %SURFACE_ALT_BG%;
        }

        QFrame#DropZone:focus {
            border: 1px solid %ACCENT%;
        }

        QFrame#DropZone[hasFile="true"] {
            border-style: solid;
            border-color: %BORDER%;
        }

        QFrame#DropZone[dragActive="true"] {
            border: 1px solid %ACCENT%;
            background-color: %ACCENT_TINT%;
        }

        QLabel#DropTitle {
            font-size: 13px;
            font-weight: 600;
            color: %TEXT_PRIMARY%;
        }

        QLabel#DropSubtitle {
            font-size: 12px;
            color: %TEXT_SECONDARY%;
        }

        QLabel#FileSizeBadge {
            background-color: %SURFACE_ALT_BG%;
            border: 1px solid %BORDER%;
            border-radius: 5px;
            padding: 2px 8px;
            font-size: 11px;
            color: %TEXT_SECONDARY%;
        }

        QFrame#SectionSurface, QFrame#ResultsPanel {
            background-color: %SURFACE_BG%;
            border: 1px solid %BORDER%;
            border-radius: 9px;
        }

        QLineEdit {
            background-color: %SURFACE_ALT_BG%;
            color: %TEXT_PRIMARY%;
            border: 1px solid %BORDER%;
            border-radius: 6px;
            padding: 6px 10px;
            selection-background-color: %ACCENT%;
            selection-color: %ACCENT_TEXT%;
        }

        QLineEdit:hover:enabled {
            border-color: %BORDER_STRONG%;
        }

        QLineEdit:focus {
            border-color: %ACCENT%;
        }

        QLineEdit:disabled {
            color: %TEXT_MUTED%;
            background-color: %WINDOW_BG%;
        }

        QPushButton {
            background-color: %SURFACE_ALT_BG%;
            color: %TEXT_PRIMARY%;
            border: 1px solid %BORDER%;
            border-radius: 6px;
            padding: 6px 12px;
            font-weight: 500;
        }

        QPushButton:hover:enabled {
            background-color: %HOVER_BG%;
            border-color: %BORDER_STRONG%;
        }

        QPushButton:pressed:enabled {
            background-color: %SURFACE_BG%;
        }

        QPushButton:focus {
            border-color: %ACCENT%;
        }

        QPushButton:disabled {
            color: %TEXT_MUTED%;
            background-color: %SURFACE_BG%;
            border-color: %BORDER%;
        }

        QPushButton#SubtleIconButton {
            background-color: transparent;
            border: 1px solid transparent;
            border-radius: 6px;
            padding: 5px 9px;
            color: %TEXT_SECONDARY%;
        }

        QPushButton#SubtleIconButton:hover:enabled {
            background-color: %HOVER_BG%;
            border-color: %BORDER%;
            color: %TEXT_PRIMARY%;
        }

        QPushButton#PrimaryButton {
            background-color: %ACCENT%;
            color: %ACCENT_TEXT%;
            border: 1px solid %ACCENT%;
            border-radius: 7px;
            padding: 9px 18px;
            font-size: 13px;
            font-weight: 600;
        }

        QPushButton#PrimaryButton:hover:enabled {
            background-color: %ACCENT_HOVER%;
            border-color: %ACCENT_HOVER%;
        }

        QPushButton#PrimaryButton:pressed:enabled {
            background-color: %ACCENT_PRESSED%;
            border-color: %ACCENT_PRESSED%;
        }

        QPushButton#PrimaryButton:disabled {
            background-color: %SURFACE_ALT_BG%;
            color: %TEXT_MUTED%;
            border-color: %BORDER%;
        }

        QPushButton#DisclosureButton {
            background-color: %SURFACE_BG%;
            color: %TEXT_SECONDARY%;
            border: 1px solid %BORDER%;
            border-radius: 7px;
            padding: 7px 12px;
            text-align: left;
            font-weight: 500;
        }

        QPushButton#DisclosureButton:hover:enabled {
            background-color: %SURFACE_ALT_BG%;
            color: %TEXT_PRIMARY%;
            border-color: %BORDER_STRONG%;
        }

        QComboBox {
            background-color: %SURFACE_ALT_BG%;
            color: %TEXT_PRIMARY%;
            border: 1px solid %BORDER%;
            border-radius: 6px;
            padding: 4px 24px 4px 10px;
            min-height: 22px;
            font-size: 12px;
        }

        QComboBox:hover:enabled {
            border-color: %BORDER_STRONG%;
            background-color: %HOVER_BG%;
        }

        QComboBox:focus {
            border-color: %ACCENT%;
        }

        QComboBox::drop-down {
            border: none;
            width: 20px;
        }

        QComboBox QAbstractItemView {
            background-color: %SURFACE_BG%;
            color: %TEXT_PRIMARY%;
            border: 1px solid %BORDER_STRONG%;
            selection-background-color: %ACCENT_TINT%;
            selection-color: %TEXT_PRIMARY%;
            outline: none;
            padding: 4px;
        }

        QProgressBar {
            background-color: %SURFACE_ALT_BG%;
            border: none;
            border-radius: 2px;
            min-height: 4px;
            max-height: 4px;
        }

        QProgressBar::chunk {
            background-color: %ACCENT%;
            border-radius: 2px;
        }

        QFrame#ErrorBanner {
            background-color: %ERROR_TINT%;
            border: 1px solid %ERROR%;
            border-radius: 7px;
        }

        QLabel#ErrorBannerText {
            color: %TEXT_PRIMARY%;
            font-size: 12px;
        }

        QLabel#StatusBadgeSuccess {
            color: %SUCCESS%;
            font-weight: 600;
            font-size: 13px;
        }

        QLabel#StatKey {
            color: %TEXT_MUTED%;
            font-size: 11px;
            font-weight: 500;
        }

        QLabel#StatValue {
            color: %TEXT_PRIMARY%;
            font-size: 13px;
            font-weight: 600;
        }

        QLabel#StatValuePositive {
            color: %SUCCESS%;
            font-size: 13px;
            font-weight: 600;
        }

        QLabel#StatValueWarning {
            color: %WARNING%;
            font-size: 13px;
            font-weight: 600;
        }

        QFrame#DividerLine {
            background-color: %BORDER%;
            min-height: 1px;
            max-height: 1px;
        }

        QTableWidget {
            background-color: %SURFACE_BG%;
            alternate-background-color: %SURFACE_ALT_BG%;
            color: %TEXT_PRIMARY%;
            gridline-color: %BORDER%;
            border: 1px solid %BORDER%;
            border-radius: 8px;
            selection-background-color: %ACCENT_TINT%;
            selection-color: %TEXT_PRIMARY%;
            outline: none;
        }

        QHeaderView::section {
            background-color: %SURFACE_ALT_BG%;
            color: %TEXT_SECONDARY%;
            border: none;
            border-bottom: 1px solid %BORDER%;
            border-right: 1px solid %BORDER%;
            padding: 6px 8px;
            font-size: 11px;
            font-weight: 600;
        }

        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 2px;
        }

        QScrollBar::handle:vertical {
            background: %BORDER_STRONG%;
            border-radius: 4px;
            min-height: 24px;
        }

        QScrollBar::handle:vertical:hover {
            background: %TEXT_MUTED%;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
            height: 0px;
        }

        QMessageBox {
            background-color: %SURFACE_BG%;
        }
    )");

    css.replace(QStringLiteral("%WINDOW_BG%"), p.windowBg);
    css.replace(QStringLiteral("%SURFACE_BG%"), p.surfaceBg);
    css.replace(QStringLiteral("%SURFACE_ALT_BG%"), p.surfaceAltBg);
    css.replace(QStringLiteral("%HOVER_BG%"), p.hoverBg);
    css.replace(QStringLiteral("%BORDER%"), p.border);
    css.replace(QStringLiteral("%BORDER_STRONG%"), p.borderStrong);
    css.replace(QStringLiteral("%TEXT_PRIMARY%"), p.textPrimary);
    css.replace(QStringLiteral("%TEXT_SECONDARY%"), p.textSecondary);
    css.replace(QStringLiteral("%TEXT_MUTED%"), p.textMuted);
    css.replace(QStringLiteral("%ACCENT%"), p.accent);
    css.replace(QStringLiteral("%ACCENT_HOVER%"), p.accentHover);
    css.replace(QStringLiteral("%ACCENT_PRESSED%"), p.accentPressed);
    css.replace(QStringLiteral("%ACCENT_TINT%"), p.accentTint);
    css.replace(QStringLiteral("%ACCENT_TEXT%"), p.accentText);
    css.replace(QStringLiteral("%SUCCESS%"), p.success);
    css.replace(QStringLiteral("%WARNING%"), p.warning);
    css.replace(QStringLiteral("%ERROR%"), p.error);
    css.replace(QStringLiteral("%ERROR_TINT%"), p.errorTint);
    return css;
}

QIcon createAppLogoIcon(const ThemePalette& palette)
{
    QIcon icon;
    const int sizes[] = {24, 32, 48, 64};
    for (int sz : sizes)
    {
        QPixmap pm = makeCanvas(sz, 2.0);
        QPainter painter(&pm);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const qreal pad = sz * 0.08;
        const qreal box = sz - 2.0 * pad;
        const qreal radius = sz * 0.22;

        painter.setPen(QPen(QColor(palette.borderStrong), qMax(1.0, sz * 0.04)));
        painter.setBrush(QColor(palette.surfaceAltBg));
        painter.drawRoundedRect(QRectF(pad, pad, box, box), radius, radius);

        // Minimal Huffman binary tree motif: root -> left leaf & right branch -> 2 leaves
        const QPointF root(sz * 0.50, sz * 0.28);
        const QPointF leftLeaf(sz * 0.30, sz * 0.56);
        const QPointF rightInternal(sz * 0.68, sz * 0.50);
        const QPointF rightLeft(sz * 0.55, sz * 0.74);
        const QPointF rightRight(sz * 0.78, sz * 0.74);

        QPen edgePen(QColor(palette.textSecondary), qMax(1.3, sz * 0.06), Qt::SolidLine, Qt::RoundCap);
        painter.setPen(edgePen);
        painter.drawLine(root, leftLeaf);
        painter.drawLine(root, rightInternal);
        painter.drawLine(rightInternal, rightLeft);
        painter.drawLine(rightInternal, rightRight);

        const qreal nodeR = sz * 0.085;
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(palette.accent));
        painter.drawEllipse(root, nodeR, nodeR);
        painter.drawEllipse(rightInternal, nodeR * 0.9, nodeR * 0.9);

        painter.setBrush(QColor(palette.textPrimary));
        painter.drawEllipse(leftLeaf, nodeR * 0.85, nodeR * 0.85);
        painter.drawEllipse(rightLeft, nodeR * 0.85, nodeR * 0.85);
        painter.drawEllipse(rightRight, nodeR * 0.85, nodeR * 0.85);

        icon.addPixmap(pm);
    }
    return icon;
}

QIcon createUploadIcon(const QColor& color)
{
    QPixmap pm = makeCanvas(28, 2.0);
    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(color, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    // Tray
    QPainterPath tray;
    tray.moveTo(5.5, 17.5);
    tray.lineTo(5.5, 21.5);
    tray.lineTo(22.5, 21.5);
    tray.lineTo(22.5, 17.5);
    painter.drawPath(tray);

    // Upward arrow
    painter.drawLine(QPointF(14.0, 16.5), QPointF(14.0, 6.5));
    painter.drawLine(QPointF(9.5, 11.0), QPointF(14.0, 6.5));
    painter.drawLine(QPointF(18.5, 11.0), QPointF(14.0, 6.5));

    return QIcon(pm);
}

QIcon createFileIcon(const QColor& color)
{
    QPixmap pm = makeCanvas(22, 2.0);
    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(color, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    QPainterPath doc;
    doc.moveTo(5.5, 3.5);
    doc.lineTo(13.0, 3.5);
    doc.lineTo(16.5, 7.0);
    doc.lineTo(16.5, 18.5);
    doc.lineTo(5.5, 18.5);
    doc.closeSubpath();
    painter.drawPath(doc);

    painter.drawLine(QPointF(13.0, 3.5), QPointF(13.0, 7.0));
    painter.drawLine(QPointF(13.0, 7.0), QPointF(16.5, 7.0));

    return QIcon(pm);
}

QIcon createFolderIcon(const QColor& color)
{
    QPixmap pm = makeCanvas(18, 2.0);
    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(color, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    QPainterPath folder;
    folder.moveTo(2.5, 4.5);
    folder.lineTo(7.0, 4.5);
    folder.lineTo(8.5, 6.2);
    folder.lineTo(15.5, 6.2);
    folder.lineTo(15.5, 14.0);
    folder.lineTo(2.5, 14.0);
    folder.closeSubpath();
    painter.drawPath(folder);

    return QIcon(pm);
}

QIcon createCloseIcon(const QColor& color)
{
    QPixmap pm = makeCanvas(16, 2.0);
    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(color, 1.5, Qt::SolidLine, Qt::RoundCap));

    painter.drawLine(QPointF(4.5, 4.5), QPointF(11.5, 11.5));
    painter.drawLine(QPointF(11.5, 4.5), QPointF(4.5, 11.5));

    return QIcon(pm);
}

QIcon createChevronIcon(const QColor& color, bool expanded)
{
    QPixmap pm = makeCanvas(16, 2.0);
    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(color, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    if (expanded)
    {
        painter.drawLine(QPointF(4.0, 6.0), QPointF(8.0, 10.0));
        painter.drawLine(QPointF(8.0, 10.0), QPointF(12.0, 6.0));
    }
    else
    {
        painter.drawLine(QPointF(6.0, 4.0), QPointF(10.0, 8.0));
        painter.drawLine(QPointF(10.0, 8.0), QPointF(6.0, 12.0));
    }

    return QIcon(pm);
}

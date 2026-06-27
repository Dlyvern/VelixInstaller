#ifndef VELIX_THEME_HPP
#define VELIX_THEME_HPP

#include <QColor>
#include <QFont>
#include <QString>

// Velix design tokens — derived from the HTML/CSS prototype in
// `Velix Installer.html`. Values are sRGB hex equivalents of the oklch()
// tokens in the prototype's :root selector.
namespace theme
{
    // ── Accent (ember by default) ─────────────────────────────────────────
    inline QColor accent          = QColor(0xE0, 0x7A, 0x3B);
    inline QColor accentSoft      = QColor(0xC8, 0x68, 0x33);
    inline QColor accentDeep      = QColor(0x8E, 0x47, 0x21);
    inline QColor accentBright    = QColor(0xF0, 0x9E, 0x5C);
    inline QColor accentMuted     = QColor(0xA8, 0x73, 0x52);
    inline QColor accentMutedDeep = QColor(0x6E, 0x42, 0x27);

    // ── Surfaces ──────────────────────────────────────────────────────────
    inline QColor bg0          = QColor(0x18, 0x16, 0x14);
    inline QColor bg1          = QColor(0x1C, 0x19, 0x17);
    inline QColor surface0     = QColor(0x23, 0x21, 0x20);
    inline QColor surface1     = QColor(0x2A, 0x26, 0x24);
    inline QColor surface1Hover= QColor(0x32, 0x2E, 0x2B);
    inline QColor surface2     = QColor(0x35, 0x31, 0x2F);
    inline QColor surface2Hover= QColor(0x3F, 0x3A, 0x37);

    // ── Text ──────────────────────────────────────────────────────────────
    inline QColor text  = QColor(0xF2, 0xEF, 0xEC);
    inline QColor text2 = QColor(0xC2, 0xBB, 0xB4);
    inline QColor text3 = QColor(0x8E, 0x85, 0x81);

    // ── Borders ───────────────────────────────────────────────────────────
    inline QColor border      = QColor(0x3A, 0x36, 0x33);
    inline QColor borderHover = QColor(0x55, 0x4E, 0x49);

    // ── Channels ──────────────────────────────────────────────────────────
    inline QColor dev     = QColor(0x92, 0x85, 0xFF);
    inline QColor preview = QColor(0xE0, 0xB2, 0x68);

    // ── Geometry ──────────────────────────────────────────────────────────
    inline constexpr int radiusCard   = 12;
    inline constexpr int radiusField  =  8;
    inline constexpr int radiusButton =  8;
    inline constexpr int radiusPill   = 999;

    // ── Helpers ───────────────────────────────────────────────────────────
    inline QColor withAlpha(QColor c, int a)
    {
        c.setAlpha(a);
        return c;
    }

    inline QFont uiFont(int pt = 10, bool bold = false)
    {
        QFont f("Inter");
        f.setStyleHint(QFont::SansSerif);
        f.setPointSize(pt);
        f.setWeight(bold ? QFont::DemiBold : QFont::Normal);
        return f;
    }

    inline QFont monoFont(int pt = 10, bool bold = false)
    {
        QFont f("JetBrains Mono");
        f.setStyleHint(QFont::Monospace);
        f.setPointSize(pt);
        f.setWeight(bold ? QFont::DemiBold : QFont::Normal);
        return f;
    }
}

#endif // VELIX_THEME_HPP

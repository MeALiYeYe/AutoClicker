#pragma once
// =============================================================================
//  FontManager.h - RAII wrapper for font creation and cleanup.
//  Creates title / content / small fonts once and reuses them.
//  Fonts are DPI-aware: call setDpi() to recreate fonts at a new DPI.
// =============================================================================

#include <windows.h>
#include <initializer_list>
#include "Theme.h"
#include "DpiHelper.h"

class FontManager
{
public:
    FontManager()
    {
        // Default to system DPI at construction time.
        createFonts(DpiHelper::getSystemDpi());
    }

    ~FontManager()
    {
        cleanupFonts();
    }

    // Recreate all fonts at a new DPI. Call on WM_DPICHANGED.
    void setDpi(UINT dpi)
    {
        createFonts(dpi);
    }

    HFONT title()     const { return hFontTitle; }
    HFONT content()   const { return hFontContent; }
    HFONT smallFont() const { return hFontSmall; }

    UINT currentDpi() const { return m_dpi; }

    // Convenience: apply a font to a control.
    static void apply(HWND hCtrl, HFONT hFont)
    {
        if (hCtrl && hFont)
            SendMessageW(hCtrl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    }

    // Apply a font to multiple controls.
    static void applyTo(std::initializer_list<HWND> controls, HFONT hFont)
    {
        for (HWND h : controls)
            apply(h, hFont);
    }

private:
    void createFonts(UINT dpi)
    {
        cleanupFonts();
        m_dpi = dpi;

        int titleSize   = DpiHelper::scaleFont(Theme::FONT_TITLE_SIZE, dpi);
        int contentSize = DpiHelper::scaleFont(Theme::FONT_CONTENT_SIZE, dpi);
        int smallSize   = DpiHelper::scaleFont(Theme::FONT_SMALL_SIZE, dpi);

        hFontTitle = CreateFontW(
            titleSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FF_DONTCARE, Theme::FONT_TITLE);

        hFontContent = CreateFontW(
            contentSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FF_DONTCARE, Theme::FONT_CONTENT);

        hFontSmall = CreateFontW(
            smallSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FF_DONTCARE, Theme::FONT_CONTENT);
    }

    void cleanupFonts()
    {
        if (hFontTitle)   { DeleteObject(hFontTitle);   hFontTitle = nullptr; }
        if (hFontContent) { DeleteObject(hFontContent); hFontContent = nullptr; }
        if (hFontSmall)   { DeleteObject(hFontSmall);   hFontSmall = nullptr; }
    }

    HFONT hFontTitle   = nullptr;
    HFONT hFontContent = nullptr;
    HFONT hFontSmall   = nullptr;
    UINT  m_dpi        = 96;
};

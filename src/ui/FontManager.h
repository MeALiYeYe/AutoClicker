#pragma once
// =============================================================================
//  FontManager.h - RAII wrapper for font creation and cleanup.
//  Creates title / content / small fonts once and reuses them.
// =============================================================================

#include <windows.h>
#include "Theme.h"

class FontManager
{
public:
    FontManager()
    {
        hFontTitle = CreateFontW(
            Theme::FONT_TITLE_SIZE, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FF_DONTCARE, Theme::FONT_TITLE);

        hFontContent = CreateFontW(
            Theme::FONT_CONTENT_SIZE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FF_DONTCARE, Theme::FONT_CONTENT);

        hFontSmall = CreateFontW(
            Theme::FONT_SMALL_SIZE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FF_DONTCARE, Theme::FONT_CONTENT);
    }

    ~FontManager()
    {
        if (hFontTitle)   DeleteObject(hFontTitle);
        if (hFontContent) DeleteObject(hFontContent);
        if (hFontSmall)   DeleteObject(hFontSmall);
    }

    HFONT title()   const { return hFontTitle; }
    HFONT content() const { return hFontContent; }
    HFONT small()   const { return hFontSmall; }

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
    HFONT hFontTitle   = nullptr;
    HFONT hFontContent = nullptr;
    HFONT hFontSmall   = nullptr;
};

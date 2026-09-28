#pragma once
// =============================================================================
//  Theme.h - Centralized UI layout constants, colors, and font names.
//  All dimensions in pixels; designed for a 420x640 window.
// =============================================================================

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace Theme
{
    // ---- Window ----
    constexpr int WINDOW_WIDTH  = 420;
    constexpr int WINDOW_HEIGHT = 640;

    // ---- Margins & spacing ----
    constexpr int MARGIN_X    = 12;
    constexpr int MARGIN_Y    = 12;
    constexpr int SMALL_GAP   = 5;
    constexpr int GROUP_GAP   = 8;

    // ---- Element sizes ----
    constexpr int ELEMENT_WIDTH  = 80;
    constexpr int ELEMENT_HEIGHT = 26;
    constexpr int BUTTON_WIDTH   = ELEMENT_WIDTH;
    constexpr int BUTTON_HEIGHT  = 30;

    // ---- Group box ----
    constexpr int GROUP_WIDTH       = 396;
    constexpr int HALF_WIDTH        = static_cast<int>((GROUP_WIDTH - SMALL_GAP * 2) * 0.5);
    constexpr int INTERVAL_GROUP_H  = 92;
    constexpr int POS_GROUP_H       = 155;
    constexpr int PROFILE_GROUP_H   = 60;
    constexpr int LIMIT_GROUP_H     = 155;
    constexpr int GROUPBOX_HEADER_H = 20;

    // ---- Control sizes ----
    constexpr int RADIO_WIDTH       = ELEMENT_WIDTH;
    constexpr int RADIO_HEIGHT      = ELEMENT_HEIGHT;
    constexpr int EDIT_WIDTH        = ELEMENT_WIDTH;
    constexpr int EDIT_HEIGHT       = ELEMENT_HEIGHT;
    constexpr int STATUS_HEIGHT     = ELEMENT_HEIGHT;
    constexpr int TYPE_GROUP_W      = 130;

    // ---- Label widths ----
    constexpr int LABEL_WIDTH       = ELEMENT_WIDTH;
    constexpr int SHORT_LABEL_WIDTH = static_cast<int>(ELEMENT_WIDTH * 0.5);
    constexpr int UNIT_WIDTH        = 25;

    // ---- Special positions ----
    constexpr int EDIT_LEFT_LIMIT_RIGHT =
        static_cast<int>(MARGIN_X + GROUP_WIDTH - SMALL_GAP * 3 - EDIT_WIDTH - UNIT_WIDTH);

    // ---- Colors (modern soft palette) ----
    constexpr COLORREF CLR_BG          = RGB(245, 247, 250);  // window background
    constexpr COLORREF CLR_GROUP_BG    = RGB(252, 253, 255);  // group box background
    constexpr COLORREF CLR_ACCENT      = RGB(66, 133, 244);   // Google blue
    constexpr COLORREF CLR_ACCENT_DARK = RGB(50, 100, 200);
    constexpr COLORREF CLR_TEXT        = RGB(42, 47, 56);
    constexpr COLORREF CLR_TEXT_LIGHT  = RGB(120, 125, 135);
    constexpr COLORREF CLR_PROGRESS_BG = RGB(225, 230, 238);
    constexpr COLORREF CLR_BORDER      = RGB(210, 215, 222);

    // Progress bar gradient endpoints.
    constexpr COLORREF CLR_GRADIENT_START = RGB(66, 133, 244);
    constexpr COLORREF CLR_GRADIENT_END   = RGB(30, 80, 180);

    // ---- Fonts ----
    inline constexpr const wchar_t* FONT_TITLE   = L"Microsoft YaHei UI Bold";
    inline constexpr const wchar_t* FONT_CONTENT = L"Microsoft YaHei UI";
    inline constexpr const wchar_t* FONT_MONO    = L"Consolas";

    constexpr int FONT_TITLE_SIZE   = 18;
    constexpr int FONT_CONTENT_SIZE = 15;
    constexpr int FONT_SMALL_SIZE   = 13;
}

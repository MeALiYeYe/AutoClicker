#pragma once
// =============================================================================
//  Theme.h - Centralized UI layout constants, colors, and font names.
//  All dimensions are design units at 96 DPI (100% scaling). They must be
//  scaled by the actual display DPI using DpiHelper::scale() before use.
//  Designed for a 420x640 window at 96 DPI.
// =============================================================================

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace Theme
{
    // ---- Window (96 DPI design units) ----
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

    // ---- Coordinate row (dynamically created inside container) ----
    constexpr int COORD_ROW_START_Y   = 25;   // Y offset from container top
    constexpr int COORD_ROW_STEP      = 30;    // Y step per row
    constexpr int COORD_LABEL_X       = 10;   // X of coordinate label
    constexpr int COORD_LABEL_W       = 100;  // Width of coordinate label
    constexpr int COORD_ROW_H         = 22;   // Height of coordinate row controls
    constexpr int COORD_EDIT_X        = 130;  // X of interval edit box
    constexpr int COORD_EDIT_W        = 50;   // Width of interval edit box
    constexpr int COORD_EDIT_X_ALT    = 110;  // Alternate X (used in hook)
    constexpr int COORD_EDIT_W_ALT    = 64;   // Alternate width (EDIT_WIDTH * 0.8)
    constexpr int COORD_LINE_Y_OFFSET = 24;   // Y offset for separator line

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

#pragma once
// =============================================================================
//  ProgressBar.h - Custom owner-drawn gradient progress bar.
//  DPI-aware: font size and corner radius are scaled by the current DPI.
// =============================================================================

#include <windows.h>
#include "Theme.h"
#include "DpiHelper.h"

class ProgressBar
{
public:
    void setPercent(int p) { percent = (p < 0) ? 0 : (p > 100) ? 100 : p; }
    int  getPercent() const { return percent; }

    // Set the DPI for font scaling. Call before paint or on WM_DPICHANGED.
    void setDpi(UINT dpi) { m_dpi = dpi; }

    // Paint the progress bar into the given HDC / HWND.
    void paint(HWND hWnd, HDC hdc) const
    {
        RECT rc;
        GetClientRect(hWnd, &rc);

        // Scale corner radius by DPI.
        int cornerRadius = DpiHelper::scale(8, m_dpi);

        // --- Background (rounded) ---
        HBRUSH hBrushBg = CreateSolidBrush(Theme::CLR_PROGRESS_BG);
        HRGN   hRgn     = CreateRoundRectRgn(rc.left, rc.top, rc.right, rc.bottom,
                                              cornerRadius, cornerRadius);
        SelectClipRgn(hdc, hRgn);
        FillRect(hdc, &rc, hBrushBg);
        DeleteObject(hBrushBg);

        // --- Progress fill (gradient) ---
        int width = (rc.right - rc.left) * percent / 100;
        if (width > 0)
        {
            RECT rcProgress = { rc.left, rc.top, rc.left + width, rc.bottom };

            TRIVERTEX vertex[2];
            vertex[0].x     = rcProgress.left;
            vertex[0].y     = rcProgress.top;
            vertex[0].Red   = GetRValue(Theme::CLR_GRADIENT_START) << 8;
            vertex[0].Green = GetGValue(Theme::CLR_GRADIENT_START) << 8;
            vertex[0].Blue  = GetBValue(Theme::CLR_GRADIENT_START) << 8;
            vertex[0].Alpha = 0x0000;

            vertex[1].x     = rcProgress.right;
            vertex[1].y     = rcProgress.bottom;
            vertex[1].Red   = GetRValue(Theme::CLR_GRADIENT_END) << 8;
            vertex[1].Green = GetGValue(Theme::CLR_GRADIENT_END) << 8;
            vertex[1].Blue  = GetBValue(Theme::CLR_GRADIENT_END) << 8;
            vertex[1].Alpha = 0x0000;

            GRADIENT_RECT gRect = { 0, 1 };
            GradientFill(hdc, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_H);
        }

        // --- Rounded border ---
        HPEN   hPen       = CreatePen(PS_SOLID, 1, Theme::CLR_BORDER);
        HBRUSH hNullBrush = static_cast<HBRUSH>(GetStockObject(HOLLOW_BRUSH));
        HPEN   hOldPen    = static_cast<HPEN>(SelectObject(hdc, hPen));
        HBRUSH hOldBrush  = static_cast<HBRUSH>(SelectObject(hdc, hNullBrush));
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom,
                  cornerRadius, cornerRadius);
        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hPen);

        SelectClipRgn(hdc, nullptr);
        DeleteObject(hRgn);

        // --- Percentage text ---
        wchar_t buf[16];
        swprintf_s(buf, L"%d%%", percent);

        // DPI-scaled font size (14px at 96 DPI).
        int fontSize = DpiHelper::scaleFont(14, m_dpi);
        HFONT hFont = CreateFontW(
            fontSize, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, Theme::CLR_TEXT);
        DrawTextW(hdc, buf, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
    }

private:
    int  percent = 0;
    UINT m_dpi  = 96;
};

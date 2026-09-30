#pragma once
// =============================================================================
//  DpiHelper.h - DPI (dots-per-inch) awareness utilities.
//  Provides functions to enable DPI awareness, query current DPI, and scale
//  pixel values from 96-DPI design units to the actual display DPI.
// =============================================================================

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// WM_DPICHANGED is defined in Windows 10 SDK; define for older SDKs.
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E3
#endif

// DPI_AWARENESS_CONTEXT handle values for SetProcessDpiAwarenessContext.
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE)-4)
#endif
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE ((HANDLE)-3)
#endif

class DpiHelper
{
public:
    /// Call once at startup to make the process DPI-aware.
    /// Tries PerMonitorV2, falls back to PerMonitor, then System DPI Aware.
    /// This is a runtime fallback for the manifest declaration.
    static void enableDpiAwareness()
    {
        HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
        if (hUser32)
        {
            typedef BOOL(WINAPI* PFN_SetProcessDpiAwarenessContext)(HANDLE);
            auto pFn = reinterpret_cast<PFN_SetProcessDpiAwarenessContext>(
                GetProcAddress(hUser32, "SetProcessDpiAwarenessContext"));
            if (pFn)
            {
                if (pFn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
                    return;
                // Try PerMonitor (v1) as fallback.
                if (pFn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
                    return;
            }
        }

        // Fallback: SetProcessDpiAwareness (Win 8.1+, shcore.dll)
        HMODULE hShcore = LoadLibraryW(L"shcore.dll");
        if (hShcore)
        {
            typedef HRESULT(WINAPI* PFN_SetProcessDpiAwareness)(int);
            auto pFn = reinterpret_cast<PFN_SetProcessDpiAwareness>(
                GetProcAddress(hShcore, "SetProcessDpiAwareness"));
            if (pFn)
            {
                // PROCESS_PER_MONITOR_DPI_AWARE = 2
                if (SUCCEEDED(pFn(2)))
                {
                    FreeLibrary(hShcore);
                    return;
                }
            }
            FreeLibrary(hShcore);
        }

        // Last resort: SetProcessDPIAware (Vista+)
        SetProcessDPIAware();
    }

    /// Get the effective DPI for a specific window.
    /// Uses GetDpiForWindow (Win10 1607+) with fallbacks.
    static UINT getDpi(HWND hWnd)
    {
        if (!hWnd)
            return getSystemDpi();

        HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
        if (hUser32)
        {
            typedef UINT(WINAPI* PFN_GetDpiForWindow)(HWND);
            auto pFn = reinterpret_cast<PFN_GetDpiForWindow>(
                GetProcAddress(hUser32, "GetDpiForWindow"));
            if (pFn)
            {
                UINT dpi = pFn(hWnd);
                if (dpi > 0)
                    return dpi;
            }
        }

        // Fallback: monitor DPI via shcore.dll
        HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
        HMODULE hShcore = LoadLibraryW(L"shcore.dll");
        if (hShcore)
        {
            typedef HRESULT(WINAPI* PFN_GetDpiForMonitor)(HMONITOR, int, UINT*, UINT*);
            auto pFn = reinterpret_cast<PFN_GetDpiForMonitor>(
                GetProcAddress(hShcore, "GetDpiForMonitor"));
            if (pFn)
            {
                UINT dpiX = 96, dpiY = 96;
                // MDT_EFFECTIVE_DPI = 0
                if (SUCCEEDED(pFn(hMon, 0, &dpiX, &dpiY)))
                {
                    FreeLibrary(hShcore);
                    return dpiY > 0 ? dpiY : 96;
                }
            }
            FreeLibrary(hShcore);
        }

        // Final fallback: GetDeviceCaps
        return getSystemDpi();
    }

    /// Get the system DPI (typically 96, 120, 144, 168, 192, etc.)
    static UINT getSystemDpi()
    {
        HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
        if (hUser32)
        {
            typedef UINT(WINAPI* PFN_GetDpiForSystem)();
            auto pFn = reinterpret_cast<PFN_GetDpiForSystem>(
                GetProcAddress(hUser32, "GetDpiForSystem"));
            if (pFn)
            {
                UINT dpi = pFn();
                if (dpi > 0)
                    return dpi;
            }
        }

        HDC hdc = GetDC(nullptr);
        if (!hdc)
            return 96;
        UINT dpi = static_cast<UINT>(GetDeviceCaps(hdc, LOGPIXELSY));
        ReleaseDC(nullptr, hdc);
        return dpi > 0 ? dpi : 96;
    }

    /// Scale a pixel value from 96-DPI design units to the given DPI.
    static int scale(int value, UINT dpi)
    {
        return MulDiv(value, static_cast<int>(dpi), 96);
    }

    /// Scale a pixel value for a specific window.
    static int scale(int value, HWND hWnd)
    {
        return scale(value, getDpi(hWnd));
    }

    /// Scale a font size from 96-DPI design units to the given DPI.
    static int scaleFont(int fontSize, UINT dpi)
    {
        return MulDiv(fontSize, static_cast<int>(dpi), 96);
    }
};

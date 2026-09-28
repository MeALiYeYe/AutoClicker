#pragma once
// =============================================================================
//  ClickPoint.h - Data model for a single configurable click point.
//  Each point has its own screen coordinates and optional independent interval.
// =============================================================================

#include <windows.h>
#include <chrono>

struct ClickPoint
{
    POINT pos = { 0, 0 };
    int   interval = 0;  // ms; 0 = use global interval

    // Per-point independent scheduling time (used by ClickEngine).
    std::chrono::steady_clock::time_point nextTime;

    // Child control handles (created by UIBuilder when the point is added).
    HWND  hEditCoord    = nullptr;
    HWND  hEditInterval = nullptr;
    HWND  hLine         = nullptr;  // separator line below this row
};

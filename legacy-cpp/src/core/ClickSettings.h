#pragma once
// =============================================================================
//  ClickSettings.h - Central data model for all user-configurable settings.
//  Passed between UI, ClickEngine, and ProfileManager.
// =============================================================================

#include <windows.h>
#include <string>
#include <vector>
#include "ClickPoint.h"

// Repetition limit mode
enum class LimitMode : DWORD
{
    ByCount   = 0,
    ByTime    = 1,
    Unlimited = 2
};

struct ClickSettings
{
    // --- Click Frequency ---
    bool useRandom      = true;   // true = fixed+offset, false = fixed only
    int  intervalFixed  = 1500;   // base interval (ms)
    int  intervalOffset = 500;    // random offset (ms)

    // --- Click Method ---
    DWORD clickMode     = 0;      // 0-6, see ClickStrategies.h

    // --- Click Position ---
    bool clickPosFixed  = true;   // true = use coordinates, false = follow cursor
    bool useAreaRandom  = false;  // jitter within a radius
    int  areaRadius     = 20;     // radius in pixels
    bool useRandomPos   = false;  // randomize point order

    // --- Repetition Limit ---
    LimitMode limitMode       = LimitMode::Unlimited;
    int  clickCountLimit      = 1000;
    int  clickDurationSec     = 3600;

    // --- Anti-addiction (rest) ---
    bool enableRest    = false;
    int  restTime      = 600;     // seconds

    // --- Multi-click points ---
    std::vector<ClickPoint> clickPoints;
};

// ---------------------------------------------------------------------------
//  SettingsProfile — a named, persistable snapshot of ClickSettings.
// ---------------------------------------------------------------------------
struct SettingsProfile
{
    std::wstring           name;
    ClickSettings          settings;
};

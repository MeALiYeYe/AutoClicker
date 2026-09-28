#pragma once
// =============================================================================
//  UIBuilder.h - Builds all Win32 child controls for the main window.
//  Populates a struct of HWND handles that MainWindow uses for interaction.
// =============================================================================

#include <windows.h>
#include <vector>
#include "../ui/Theme.h"
#include "../ui/FontManager.h"
#include "../core/ClickPoint.h"

// All control handles grouped for clarity.
struct UIControls
{
    // --- Click Frequency ---
    HWND hGroupFreq    = nullptr;
    HWND hRadioFixed   = nullptr;
    HWND hRadioRandom  = nullptr;
    HWND hEditCurrent  = nullptr;
    HWND hEditRandom   = nullptr;

    // --- Click Position ---
    HWND hGroupPos     = nullptr;
    HWND hRadioFixedPos = nullptr;
    HWND hRadioFreePos  = nullptr;
    HWND hRadioSeq     = nullptr;
    HWND hRadioRandPos = nullptr;
    HWND hCheckRandomArea = nullptr;
    HWND hLblRadius    = nullptr;
    HWND hEditAreaRadius = nullptr;
    HWND hBtnPick      = nullptr;
    HWND hBtnClearPos  = nullptr;

    // --- Coordinate list container ---
    HWND hGroupPosContainer = nullptr;

    // --- Repetition Limit ---
    HWND hGroupLimit   = nullptr;
    HWND hRadioLimitForever = nullptr;
    HWND hRadioLimitCount   = nullptr;
    HWND hEditTimes        = nullptr;
    HWND hRadioLimitTime    = nullptr;
    HWND hEditDuration     = nullptr;
    HWND hCheckRest        = nullptr;
    HWND hEditRestTime     = nullptr;

    // --- Click Method ---
    HWND hGroupClick   = nullptr;
    HWND hComboButton  = nullptr;
    HWND hComboClickType = nullptr;

    // --- Profile ---
    HWND hGroupProfile  = nullptr;
    HWND hComboProfiles = nullptr;
    HWND hBtnSaveProfile = nullptr;
    HWND hBtnDeleteProfile = nullptr;

    // --- Control bar ---
    HWND hGroupCtrl    = nullptr;
    HWND hBtnStartPause = nullptr;
    HWND hBtnStop       = nullptr;
    HWND hBtnSetHotkey  = nullptr;
    HWND hStaticHotkey  = nullptr;

    // --- Status ---
    HWND hStatus         = nullptr;
    HWND hStatusInterval = nullptr;
    HWND hStatusCountTime = nullptr;

    // --- Progress ---
    HWND hLblProgress = nullptr;
    HWND hProgress    = nullptr;
};

class UIBuilder
{
public:
    // Build the entire UI inside hWnd. Returns a populated UIControls.
    static UIControls build(HWND hWnd, const FontManager& fonts);

    // Create a separator line (static with SS_ETCHEDHORZ).
    static HWND createSeparator(HWND parent, int x, int y, int width);

    // Create a labeled coordinate row inside the container group.
    static void createCoordRow(UIControls& ui, int index, const ClickPoint& pt,
                               const FontManager& fonts);

    // Destroy all coordinate-row child controls and clear the vector.
    static void clearCoordRows(UIControls& ui,
                               std::vector<ClickPoint>& clickPoints);
};

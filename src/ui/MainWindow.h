#pragma once
// =============================================================================
//  MainWindow.h - The central class that encapsulates the entire application.
//  Owns the window handle, UI controls, click engine, tray icon, hotkey
//  manager, profile manager, and fonts. Dispatches Win32 messages to
//  dedicated handler methods.
// =============================================================================

#include <windows.h>
#include <string>
#include <vector>

#include "../core/ClickEngine.h"
#include "../core/ClickSettings.h"
#include "../core/ClickPoint.h"
#include "../ui/UIBuilder.h"
#include "../ui/FontManager.h"
#include "../ui/ProgressBar.h"
#include "../ui/Theme.h"
#include "../utils/TrayIconManager.h"
#include "../utils/HotkeyManager.h"
#include "../utils/ProfileManager.h"

class MainWindow
{
public:
    MainWindow();
    ~MainWindow();

    // Create the window and all child controls.
    bool create(HINSTANCE hInstance, int nCmdShow);

    // Run the message loop (blocks until window is destroyed).
    int  runMessageLoop();

    // Static WndProc — dispatches to the instance via GWLP_USERDATA.
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    // Instance-level message handler.
    LRESULT handle(UINT msg, WPARAM wParam, LPARAM lParam);

    // ---- Message handlers ----
    void    onCreate();
    void    onCommand(WPARAM wParam, LPARAM lParam);
    void    onHotkey(WPARAM wParam);
    void    onDrawItem(LPARAM lParam);
    LRESULT onCtlColor(WPARAM wParam, LPARAM lParam);
    void    onTrayIcon(WPARAM wParam, LPARAM lParam);
    void    onDestroy();
    void    onEngineStatus(LPARAM lParam);
    void    onEngineInterval(WPARAM wParam);
    void    onEngineCountTime(LPARAM lParam);
    void    onEngineProgress(WPARAM wParam);

    // ---- Actions ----
    void    startClicking();
    void    stopClicking();
    void    toggleClicking();
    ClickSettings readSettingsFromUI();
    void    loadProfileToUI(const SettingsProfile& profile);
    void    saveCurrentProfile();
    void    deleteSelectedProfile();
    void    onPickPosition();
    void    onClearPositions();
    void    onSetHotkey();
    void    hideToTray();
    void    restoreFromTray();

    // ---- Helpers ----
    DWORD   getClickModeFromUI() const;
    void    updateHotkeyDisplay();

    // ---- Mouse hook for position picking ----
    static LRESULT CALLBACK mouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

private:
    // ---- Window ----
    HWND       m_hWnd = nullptr;
    HINSTANCE  m_hInst = nullptr;
    bool       m_hiddenToTray = false;

    // ---- UI ----
    UIControls       m_ui;
    FontManager      m_fonts;
    ProgressBar      m_progressBar;

    // ---- Engine & managers ----
    ClickEngine       m_engine;
    TrayIconManager   m_tray;
    HotkeyManager     m_hotkey;
    ProfileManager    m_profiles;

    // ---- Application state ----
    std::vector<SettingsProfile> m_profileList;
    std::vector<ClickPoint>      m_clickPoints;
    bool                         m_pickingPos = false;
    HHOOK                        m_hMouseHook = nullptr;

    // ---- Background brush for custom window color ----
    HBRUSH                       m_hBgBrush = nullptr;
};

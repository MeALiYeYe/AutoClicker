#pragma once
// =============================================================================
//  ClickEngine.h - Encapsulates the background click thread and scheduling.
//  Owns the ClickStrategy (polymorphic), reads from ClickSettings,
//  and posts WM_APP messages to the main window for UI updates.
// =============================================================================

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <functional>
#include <windows.h>
#include "ClickSettings.h"
#include "ClickStrategy.h"

// Custom messages posted to the main window (wParam = payload).
#define WM_ENGINE_STATUS      (WM_APP + 10)  // lParam = const wchar_t*
#define WM_ENGINE_INTERVAL    (WM_APP + 11)  // wParam = interval ms
#define WM_ENGINE_COUNTTIME   (WM_APP + 12)  // lParam = const wchar_t*
#define WM_ENGINE_PROGRESS    (WM_APP + 13)  // wParam = percent 0-100

class ClickEngine
{
public:
    ClickEngine();
    ~ClickEngine();

    // ---- Lifecycle ----
    void start(const ClickSettings& settings);
    void pause();
    void stop();
    bool isRunning() const { return running.load(); }

    // ---- Callback / target window ----
    void setNotifyWindow(HWND hWnd) { hNotifyWnd = hWnd; }

    // ---- Progress query (thread-safe) ----
    int  getProgress() const { return progressPercent.load(); }

private:
    // The worker thread entry point.
    void workerThread(ClickSettings settings);

    // Compute the next interval for a given base value.
    int  computeInterval(int base, const ClickSettings& s) const;

    // Apply area-random jitter to a click position.
    POINT applyAreaJitter(POINT pos, int radius) const;

    // Post a UI update message to the notify window.
    void notifyStatus(const wchar_t* msg);
    void notifyInterval(int ms);
    void notifyCountTime(const wchar_t* msg);
    void notifyProgress(int percent);

    // ---- State ----
    std::atomic<bool>        running{ false };
    std::atomic<bool>        stopFlag{ false };
    std::thread              worker;
    HWND                     hNotifyWnd = nullptr;
    std::atomic<int>         progressPercent{ 0 };

    // Counters (only accessed from worker thread — no atomics needed).
    int                      cycleCount  = 0;
    int                      totalCount  = 0;
    std::chrono::steady_clock::time_point cycleStart;
    std::chrono::steady_clock::time_point taskStart;
};

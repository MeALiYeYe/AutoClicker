#pragma once
// =============================================================================
//  HotkeyManager.h - Register/unregister global hotkeys and capture new keys.
// =============================================================================

#include <windows.h>
#include <thread>
#include <atomic>
#include <chrono>

class HotkeyManager
{
public:
    static constexpr UINT HOTKEY_ID = 1;

    HotkeyManager() = default;

    // Register the current hotkey with the OS.
    void registerHotkey(HWND hWnd)
    {
        UnregisterHotKey(hWnd, HOTKEY_ID);
        RegisterHotKey(hWnd, HOTKEY_ID, 0, currentVK);
    }

    void unregisterHotkey(HWND hWnd)
    {
        UnregisterHotKey(hWnd, HOTKEY_ID);
    }

    // Get the VK code name for display.
    static const wchar_t* vkToName(UINT vk)
    {
        // Common function keys F1-F24
        if (vk >= VK_F1 && vk <= VK_F24)
        {
            static wchar_t buf[8];
            swprintf_s(buf, L"F%d", vk - VK_F1 + 1);
            return buf;
        }
        // Single printable ASCII
        if (vk >= 0x30 && vk <= 0x5A)
        {
            static wchar_t buf[2];
            buf[0] = static_cast<wchar_t>(vk);
            buf[1] = 0;
            return buf;
        }
        return L"?";
    }

    UINT getVK() const { return currentVK; }
    void setVK(UINT vk) { currentVK = vk; }

    // Launch a background thread that waits for the user to press any key,
    // then posts WM_APP+1 to the window with the captured VK code.
    void beginCapture(HWND hWnd)
    {
        waitingForHotkey = true;
        std::thread([this, hWnd]()
        {
            while (waitingForHotkey.load())
            {
                for (int vk = 1; vk <= 254; ++vk)
                {
                    if (GetAsyncKeyState(vk) & 0x8000)
                    {
                        currentVK = static_cast<UINT>(vk);
                        waitingForHotkey = false;
                        PostMessage(hWnd, WM_APP + 1, 0, 0);
                        return;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }).detach();
    }

    void cancelCapture() { waitingForHotkey = false; }

private:
    UINT             currentVK = VK_F6;
    std::atomic<bool> waitingForHotkey{ false };
};

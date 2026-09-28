#pragma once
// =============================================================================
//  TrayIconManager.h - Encapsulates system tray icon lifecycle and menu.
// =============================================================================

#include <windows.h>
#include <shellapi.h>

#define WM_TRAYICON       (WM_USER + 1)
#define ID_TRAY_SHOW      3001
#define ID_TRAY_TOGGLE    3002
#define ID_TRAY_EXIT      3003

class TrayIconManager
{
public:
    TrayIconManager() = default;
    ~TrayIconManager() { remove(); }

    void add(HWND hWnd, HICON hIcon, const wchar_t* tip)
    {
        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize           = sizeof(NOTIFYICONDATAW);
        nid.hWnd             = hWnd;
        nid.uID              = 1;
        nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;
        nid.hIcon            = hIcon;
        wcscpy_s(nid.szTip, tip);
        Shell_NotifyIconW(NIM_ADD, &nid);
        bAdded = true;
    }

    void remove()
    {
        if (bAdded)
        {
            Shell_NotifyIconW(NIM_DELETE, &nid);
            bAdded = false;
        }
    }

    void updateTip(const wchar_t* tip)
    {
        wcscpy_s(nid.szTip, tip);
        Shell_NotifyIconW(NIM_MODIFY, &nid);
    }

    // Show the right-click context menu.
    void showMenu(HWND hWnd, bool isRunning) const
    {
        POINT pt;
        GetCursorPos(&pt);
        HMENU hMenu = CreatePopupMenu();
        InsertMenuW(hMenu, -1, MF_BYPOSITION, ID_TRAY_SHOW,   L"\x663E\x793A\x7A97\x53E3");       // "显示窗口"
        InsertMenuW(hMenu, -1, MF_BYPOSITION, ID_TRAY_TOGGLE,
                    isRunning ? L"\x6682\x505C" : L"\x5F00\x59CB");                                // "暂停" / "开始"
        InsertMenuW(hMenu, -1, MF_BYPOSITION, ID_TRAY_EXIT,   L"\x9000\x51FA");                   // "退出"
        SetForegroundWindow(hWnd);
        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, nullptr);
        DestroyMenu(hMenu);
    }

    NOTIFYICONDATAW* data() { return &nid; }

private:
    NOTIFYICONDATAW nid;
    bool            bAdded = false;
};

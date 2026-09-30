// =============================================================================
//  MainWindow.cpp - Implementation of the main application window.
//  DPI-aware: scales all dimensions by the current display DPI and
//  handles WM_DPICHANGED to rebuild the UI when the DPI changes.
// =============================================================================

#include "MainWindow.h"
#include "../core/ClickStrategies.h"
#include "resource.h"
#include <commctrl.h>
#include <shellapi.h>
#include <random>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Msimg32.lib")

// ---------------------------------------------------------------------------
//  Constructor / Destructor
// ---------------------------------------------------------------------------
MainWindow::MainWindow()
{
    m_hBgBrush = CreateSolidBrush(Theme::CLR_BG);
}

MainWindow::~MainWindow()
{
    if (m_hMouseHook)
        UnhookWindowsHookEx(m_hMouseHook);
    if (m_hBgBrush)
        DeleteObject(m_hBgBrush);
}

// ---------------------------------------------------------------------------
//  create — register window class, create window, build UI, start engine.
// ---------------------------------------------------------------------------
bool MainWindow::create(HINSTANCE hInstance, int nCmdShow)
{
    m_hInst = hInstance;

    // Get system DPI for initial window creation. The actual window DPI
    // will be queried in onCreate() and updated via WM_DPICHANGED.
    m_currentDpi = DpiHelper::getSystemDpi();

    // Set font DPI to match.
    m_fonts.setDpi(m_currentDpi);

    // Scale window dimensions to the current DPI.
    int wndW = DpiHelper::scale(Theme::WINDOW_WIDTH, m_currentDpi);
    int wndH = DpiHelper::scale(Theme::WINDOW_HEIGHT, m_currentDpi);

    // Register window class.
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc   = MainWindow::WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"AutoClickerWnd";
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = m_hBgBrush;
    wc.hIcon         = static_cast<HICON>(LoadImageW(
        hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
        GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0));
    wc.hIconSm       = static_cast<HICON>(LoadImageW(
        hInstance, MAKEINTRESOURCEW(IDI_SMALL_ICON), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));

    RegisterClassExW(&wc);

    // Create window — pass `this` as lpCreateParams for WndProc to retrieve.
    // Dimensions are DPI-scaled physical pixels.
    m_hWnd = CreateWindowExW(
        0, L"AutoClickerWnd", L"AutoClicker",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wndW, wndH,
        nullptr, nullptr, hInstance, this);

    if (!m_hWnd)
        return false;

    // Keep window on top.
    SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    // Register hotkey.
    m_hotkey.registerHotkey(m_hWnd);

    // Add tray icon (scale 16 to current DPI).
    int iconSize = DpiHelper::scale(16, m_currentDpi);
    HICON hTrayIcon = static_cast<HICON>(LoadImageW(
        hInstance, MAKEINTRESOURCEW(IDI_SMALL_ICON), IMAGE_ICON, iconSize, iconSize, 0));
    m_tray.add(m_hWnd, hTrayIcon, L"AutoClicker");

    // Connect engine to our window for notifications.
    m_engine.setNotifyWindow(m_hWnd);

    // Show window.
    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    return true;
}

// ---------------------------------------------------------------------------
//  runMessageLoop — standard Win32 message pump.
// ---------------------------------------------------------------------------
int MainWindow::runMessageLoop()
{
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

// ---------------------------------------------------------------------------
//  WndProc — static trampoline; retrieves the MainWindow instance from
//  GWLP_USERDATA (set during WM_NCCREATE) and dispatches to handle().
// ---------------------------------------------------------------------------
LRESULT CALLBACK MainWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MainWindow* pThis = nullptr;

    if (msg == WM_NCCREATE)
    {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hWnd = hWnd;
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    else
    {
        pThis = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    if (pThis)
        return pThis->handle(msg, wParam, lParam);

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
//  handle — dispatch a message to the appropriate handler method.
// ---------------------------------------------------------------------------
LRESULT MainWindow::handle(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        onCreate();
        return 0;

    case WM_COMMAND:
        onCommand(wParam, lParam);
        return 0;

    case WM_HOTKEY:
        onHotkey(wParam);
        return 0;

    case WM_APP + 1:
        // Hotkey capture completed.
        m_hotkey.registerHotkey(m_hWnd);
        updateHotkeyDisplay();
        return 0;

    // ---- DPI change ----
    case WM_DPICHANGED:
        onDpiChanged(wParam, lParam);
        return 0;

    // ---- Engine notifications ----
    case WM_ENGINE_STATUS:
        onEngineStatus(lParam);
        return 0;
    case WM_ENGINE_INTERVAL:
        onEngineInterval(wParam);
        return 0;
    case WM_ENGINE_COUNTTIME:
        onEngineCountTime(lParam);
        return 0;
    case WM_ENGINE_PROGRESS:
        onEngineProgress(wParam);
        return 0;

    // ---- Tray icon ----
    case WM_TRAYICON:
        onTrayIcon(wParam, lParam);
        return 0;

    // ---- Custom drawing ----
    case WM_CTLCOLORSTATIC:
        return onCtlColor(wParam, lParam);

    case WM_DRAWITEM:
        onDrawItem(lParam);
        return TRUE;

    case WM_DESTROY:
        onDestroy();
        return 0;

    default:
        return DefWindowProcW(m_hWnd, msg, wParam, lParam);
    }
}

// ===========================================================================
//  Message Handlers
// ===========================================================================

void MainWindow::onCreate()
{
    // Get the actual window DPI (may differ from system DPI if the window
    // was created on a monitor with a different scaling factor).
    UINT windowDpi = DpiHelper::getDpi(m_hWnd);
    if (windowDpi != m_currentDpi)
    {
        m_currentDpi = windowDpi;
        m_fonts.setDpi(m_currentDpi);
    }

    // Build the UI with DPI-scaled dimensions.
    m_ui = UIBuilder::build(m_hWnd, m_fonts);

    // Load saved profiles.
    m_profileList = m_profiles.loadAll();
    for (const auto& p : m_profileList)
        SendMessageW(m_ui.hComboProfiles, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(p.name.c_str()));
}

void MainWindow::onCommand(WPARAM wParam, LPARAM lParam)
{
    HWND hCtrl = reinterpret_cast<HWND>(lParam);

    if (hCtrl == m_ui.hBtnPick)
        onPickPosition();
    else if (hCtrl == m_ui.hBtnClearPos)
        onClearPositions();
    else if (hCtrl == m_ui.hBtnStartPause)
        toggleClicking();
    else if (hCtrl == m_ui.hBtnStop)
        stopClicking();
    else if (hCtrl == m_ui.hBtnSetHotkey)
        onSetHotkey();
    else if (hCtrl == m_ui.hBtnSaveProfile)
        saveCurrentProfile();
    else if (hCtrl == m_ui.hBtnDeleteProfile)
        deleteSelectedProfile();
    else if (HIWORD(wParam) == CBN_SELCHANGE && hCtrl == m_ui.hComboProfiles)
    {
        int sel = static_cast<int>(SendMessageW(m_ui.hComboProfiles, CB_GETCURSEL, 0, 0));
        if (sel != CB_ERR && sel < static_cast<int>(m_profileList.size()))
            loadProfileToUI(m_profileList[sel]);
    }
    else if (wParam == ID_TRAY_SHOW)
        restoreFromTray();
    else if (wParam == ID_TRAY_TOGGLE)
        toggleClicking();
    else if (wParam == ID_TRAY_EXIT)
    {
        stopClicking();
        DestroyWindow(m_hWnd);
    }
}

void MainWindow::onHotkey(WPARAM wParam)
{
    if (wParam == HotkeyManager::HOTKEY_ID)
        toggleClicking();
}

void MainWindow::onDrawItem(LPARAM lParam)
{
    auto dis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
    if (dis->CtlID == 1001)
        m_progressBar.paint(dis->hwndItem, dis->hDC);
}

LRESULT MainWindow::onCtlColor(WPARAM wParam, LPARAM lParam)
{
    HDC hdc = reinterpret_cast<HDC>(wParam);
    HWND hCtrl = reinterpret_cast<HWND>(lParam);

    // Transparent background for static text and group boxes.
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, Theme::CLR_TEXT);

    // Group boxes get a slightly different background.
    wchar_t className[32];
    GetClassNameW(hCtrl, className, 32);
    if (wcscmp(className, L"button") == 0)
    {
        // Check if it's a group box (BS_GROUPBOX style).
        LONG style = GetWindowLongW(hCtrl, GWL_STYLE);
        if (style & BS_GROUPBOX)
        {
            SetTextColor(hdc, Theme::CLR_ACCENT_DARK);
            return reinterpret_cast<LRESULT>(m_hBgBrush);
        }
    }

    return reinterpret_cast<LRESULT>(m_hBgBrush);
}

void MainWindow::onTrayIcon(WPARAM wParam, LPARAM lParam)
{
    if (lParam == WM_RBUTTONUP)
    {
        m_tray.showMenu(m_hWnd, m_engine.isRunning());
    }
    else if (lParam == WM_LBUTTONDBLCLK)
    {
        if (m_hiddenToTray)
            restoreFromTray();
        else
            hideToTray();
    }
}

void MainWindow::onDestroy()
{
    stopClicking();
    m_hotkey.unregisterHotkey(m_hWnd);
    m_tray.remove();
    PostQuitMessage(0);
}

// ---- DPI change handler ----

void MainWindow::onDpiChanged(WPARAM wParam, LPARAM lParam)
{
    UINT newDpi = LOWORD(wParam);
    if (newDpi == m_currentDpi)
        return;

    // Resize window to the system-suggested rectangle.
    auto* prcNewWindow = reinterpret_cast<RECT*>(lParam);
    SetWindowPos(m_hWnd, nullptr,
        prcNewWindow->left, prcNewWindow->top,
        prcNewWindow->right - prcNewWindow->left,
        prcNewWindow->bottom - prcNewWindow->top,
        SWP_NOZORDER | SWP_NOACTIVATE);

    // Update DPI and recreate fonts.
    m_currentDpi = newDpi;
    m_fonts.setDpi(newDpi);

    // If the UI hasn't been built yet, nothing to rebuild.
    // WM_CREATE will build it at the correct DPI.
    if (m_ui.hGroupFreq == nullptr)
        return;

    // Save current UI state before rebuilding.
    ClickSettings savedSettings = readSettingsFromUI();
    wchar_t profileName[100] = {0};
    if (m_ui.hComboProfiles)
        GetWindowTextW(m_ui.hComboProfiles, profileName, 100);

    // Rebuild all child controls at the new DPI.
    rebuildUi();

    // Re-populate the profile combo box.
    reloadProfilesToUi();

    // Restore UI state from saved settings.
    SettingsProfile profile;
    profile.name = profileName;
    profile.settings = savedSettings;
    loadProfileToUI(profile);

    // Restore the profile combo text (loadProfileToUI doesn't set it).
    if (m_ui.hComboProfiles)
        SetWindowTextW(m_ui.hComboProfiles, profileName);

    // Force a full redraw.
    InvalidateRect(m_hWnd, nullptr, TRUE);
}

void MainWindow::rebuildUi()
{
    // Destroy all child controls.
    UIBuilder::destroyAllChildren(m_hWnd);

    // Clear click points (handles are now invalid).
    m_clickPoints.clear();

    // Reset the UIControls struct.
    m_ui = UIControls{};

    // Rebuild the UI at the current DPI.
    m_ui = UIBuilder::build(m_hWnd, m_fonts);
}

void MainWindow::reloadProfilesToUi()
{
    if (!m_ui.hComboProfiles)
        return;

    SendMessageW(m_ui.hComboProfiles, CB_RESETCONTENT, 0, 0);
    for (const auto& p : m_profileList)
        SendMessageW(m_ui.hComboProfiles, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(p.name.c_str()));
}

// ---- Engine notification handlers ----

void MainWindow::onEngineStatus(LPARAM lParam)
{
    if (m_ui.hStatus)
    {
        auto msg = reinterpret_cast<const wchar_t*>(lParam);
        SetWindowTextW(m_ui.hStatus, msg);
    }
}

void MainWindow::onEngineInterval(WPARAM wParam)
{
    if (m_ui.hStatusInterval)
    {
        wchar_t buf[64];
        wsprintfW(buf, L"\x95F4\x9694: %d ms", static_cast<int>(wParam));  // "间隔: N ms"
        SetWindowTextW(m_ui.hStatusInterval, buf);
    }
}

void MainWindow::onEngineCountTime(LPARAM lParam)
{
    if (m_ui.hStatusCountTime)
    {
        auto msg = reinterpret_cast<const wchar_t*>(lParam);
        SetWindowTextW(m_ui.hStatusCountTime, msg);
    }
}

void MainWindow::onEngineProgress(WPARAM wParam)
{
    m_progressBar.setPercent(static_cast<int>(wParam));
    m_progressBar.setDpi(m_currentDpi);
    if (m_ui.hProgress)
        InvalidateRect(m_ui.hProgress, nullptr, TRUE);
}

// ===========================================================================
//  Actions
// ===========================================================================

void MainWindow::startClicking()
{
    if (m_engine.isRunning())
        return;

    ClickSettings settings = readSettingsFromUI();
    m_engine.start(settings);

    SetWindowTextW(m_ui.hStatus, L"\x72B6\x6001: \x6B63\x5728\x70B9\x51FB...");  // "状态: 正在点击..."
    SetWindowTextW(m_ui.hStatusInterval, L"\x95F4\x9694: -");  // "间隔: -"
    m_progressBar.setPercent(0);
    InvalidateRect(m_ui.hProgress, nullptr, TRUE);
}

void MainWindow::stopClicking()
{
    m_engine.stop();
    SetWindowTextW(m_ui.hStatus, L"\x72B6\x6001: \x5DF2\x505C\x6B62");  // "状态: 已停止"
    SetWindowTextW(m_ui.hStatusInterval, L"\x95F4\x9694: -");  // "间隔: -"
    m_progressBar.setPercent(0);
    InvalidateRect(m_ui.hProgress, nullptr, TRUE);
}

void MainWindow::toggleClicking()
{
    if (m_engine.isRunning())
        stopClicking();
    else
        startClicking();
}

// ---------------------------------------------------------------------------
//  readSettingsFromUI — extract all settings from UI controls into a struct.
// ---------------------------------------------------------------------------
ClickSettings MainWindow::readSettingsFromUI()
{
    ClickSettings s;
    wchar_t buf[64];

    // Frequency mode
    bool isFixed = (SendMessageW(m_ui.hRadioFixed, BM_GETCHECK, 0, 0) == BST_CHECKED);
    s.useRandom = !isFixed;

    GetWindowTextW(m_ui.hEditCurrent, buf, 64);
    s.intervalFixed = std::max(1, _wtoi(buf));

    GetWindowTextW(m_ui.hEditRandom, buf, 64);
    s.intervalOffset = std::abs(_wtoi(buf));

    // Position mode
    s.clickPosFixed = (SendMessageW(m_ui.hRadioFixedPos, BM_GETCHECK, 0, 0) == BST_CHECKED);
    s.useRandomPos  = (SendMessageW(m_ui.hRadioRandPos, BM_GETCHECK, 0, 0) == BST_CHECKED);
    s.useAreaRandom = (m_ui.hCheckRandomArea &&
                       SendMessageW(m_ui.hCheckRandomArea, BM_GETCHECK, 0, 0) == BST_CHECKED);
    if (m_ui.hEditAreaRadius)
    {
        GetWindowTextW(m_ui.hEditAreaRadius, buf, 64);
        s.areaRadius = std::max(0, _wtoi(buf));
    }

    // Per-point intervals from UI edits
    for (auto& pt : m_clickPoints)
    {
        if (pt.hEditInterval)
        {
            GetWindowTextW(pt.hEditInterval, buf, 64);
            pt.interval = std::max(0, _wtoi(buf));
        }
    }
    s.clickPoints = m_clickPoints;

    // Rest / anti-addiction
    s.enableRest = (m_ui.hCheckRest && SendMessageW(m_ui.hCheckRest, BM_GETCHECK, 0, 0) == BST_CHECKED);
    if (s.enableRest && m_ui.hEditRestTime)
    {
        GetWindowTextW(m_ui.hEditRestTime, buf, 64);
        s.restTime = std::max(0, _wtoi(buf));
    }
    else
        s.restTime = 0;

    // Count / duration limits
    if (m_ui.hEditTimes)
    {
        GetWindowTextW(m_ui.hEditTimes, buf, 64);
        s.clickCountLimit = _wtoi(buf);
    }
    if (m_ui.hEditDuration)
    {
        GetWindowTextW(m_ui.hEditDuration, buf, 64);
        s.clickDurationSec = _wtoi(buf);
    }

    // Click mode
    s.clickMode = getClickModeFromUI();

    // Limit mode
    if (SendMessageW(m_ui.hRadioLimitCount, BM_GETCHECK, 0, 0) == BST_CHECKED)
        s.limitMode = LimitMode::ByCount;
    else if (SendMessageW(m_ui.hRadioLimitTime, BM_GETCHECK, 0, 0) == BST_CHECKED)
        s.limitMode = LimitMode::ByTime;
    else
        s.limitMode = LimitMode::Unlimited;

    return s;
}

// ---------------------------------------------------------------------------
//  getClickModeFromUI — maps combo box selections to clickMode integer.
// ---------------------------------------------------------------------------
DWORD MainWindow::getClickModeFromUI() const
{
    DWORD btnSel = static_cast<DWORD>(SendMessageW(m_ui.hComboButton, CB_GETCURSEL, 0, 0));
    DWORD typeSel = static_cast<DWORD>(SendMessageW(m_ui.hComboClickType, CB_GETCURSEL, 0, 0));

    switch (btnSel)
    {
    case 0: return (typeSel == 0) ? 0 : 2;  // Left: single / double
    case 1: return (typeSel == 0) ? 1 : 3;  // Right: single / double
    case 2: return 6;                         // Middle
    case 3: return 4;                         // Wheel up
    case 4: return 5;                         // Wheel down
    default: return 0;
    }
}

// ---------------------------------------------------------------------------
//  loadProfileToUI — apply a saved profile's settings to the UI controls.
//  Coordinate rows are created with DPI-scaled positions.
// ---------------------------------------------------------------------------
void MainWindow::loadProfileToUI(const SettingsProfile& profile)
{
    const auto& s = profile.settings;

    // Clear existing coordinate rows.
    UIBuilder::clearCoordRows(m_ui, m_clickPoints);

    // Frequency
    SendMessageW(s.useRandom ? m_ui.hRadioRandom : m_ui.hRadioFixed, BM_SETCHECK, BST_CHECKED, 0);
    SetWindowTextW(m_ui.hEditCurrent, std::to_wstring(s.intervalFixed).c_str());
    SetWindowTextW(m_ui.hEditRandom, std::to_wstring(s.intervalOffset).c_str());

    // Click method
    int btnSel = 0, typeSel = 0;
    if (s.clickMode == 1) btnSel = 1;
    if (s.clickMode == 2) typeSel = 1;
    if (s.clickMode == 3) { btnSel = 1; typeSel = 1; }
    if (s.clickMode == 6) btnSel = 2;
    if (s.clickMode == 4) btnSel = 3;
    if (s.clickMode == 5) btnSel = 4;
    SendMessageW(m_ui.hComboButton, CB_SETCURSEL, btnSel, 0);
    SendMessageW(m_ui.hComboClickType, CB_SETCURSEL, typeSel, 0);

    // Position
    SendMessageW(s.clickPosFixed ? m_ui.hRadioFixedPos : m_ui.hRadioFreePos, BM_SETCHECK, BST_CHECKED, 0);
    SendMessageW(s.useRandomPos ? m_ui.hRadioRandPos : m_ui.hRadioSeq, BM_SETCHECK, BST_CHECKED, 0);
    SendMessageW(m_ui.hCheckRandomArea, BM_SETCHECK, s.useAreaRandom ? BST_CHECKED : BST_UNCHECKED, 0);
    SetWindowTextW(m_ui.hEditAreaRadius, std::to_wstring(s.areaRadius).c_str());

    // Limit mode
    if (s.limitMode == LimitMode::ByCount)
        SendMessageW(m_ui.hRadioLimitCount, BM_SETCHECK, BST_CHECKED, 0);
    else if (s.limitMode == LimitMode::ByTime)
        SendMessageW(m_ui.hRadioLimitTime, BM_SETCHECK, BST_CHECKED, 0);
    else
        SendMessageW(m_ui.hRadioLimitForever, BM_SETCHECK, BST_CHECKED, 0);
    SetWindowTextW(m_ui.hEditTimes, std::to_wstring(s.clickCountLimit).c_str());
    SetWindowTextW(m_ui.hEditDuration, std::to_wstring(s.clickDurationSec).c_str());

    // Rest
    SendMessageW(m_ui.hCheckRest, BM_SETCHECK, s.enableRest ? BST_CHECKED : BST_UNCHECKED, 0);
    SetWindowTextW(m_ui.hEditRestTime, std::to_wstring(s.restTime).c_str());

    // Recreate coordinate rows with DPI-scaled positions.
    for (const auto& pt : s.clickPoints)
    {
        ClickPoint newPt = pt;
        int index = static_cast<int>(m_clickPoints.size()) + 1;

        // DPI-scaled positions.
        int baseY = DpiHelper::scale(Theme::COORD_ROW_START_Y, m_currentDpi)
                  + (index - 1) * DpiHelper::scale(Theme::COORD_ROW_STEP, m_currentDpi);
        int labelX = DpiHelper::scale(Theme::COORD_LABEL_X, m_currentDpi);
        int labelW = DpiHelper::scale(Theme::COORD_LABEL_W, m_currentDpi);
        int rowH   = DpiHelper::scale(Theme::COORD_ROW_H, m_currentDpi);
        int editX  = DpiHelper::scale(Theme::COORD_EDIT_X, m_currentDpi);
        int editW  = DpiHelper::scale(Theme::COORD_EDIT_W, m_currentDpi);
        int lineY  = baseY + DpiHelper::scale(Theme::COORD_LINE_Y_OFFSET, m_currentDpi);
        int editLeftLimitRight = DpiHelper::scale(Theme::EDIT_LEFT_LIMIT_RIGHT, m_currentDpi);

        wchar_t buf[128];

        wsprintfW(buf, L"%d. (%d, %d)", index, newPt.pos.x, newPt.pos.y);
        newPt.hEditCoord = CreateWindowW(L"static", buf,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            labelX, baseY, labelW, rowH, m_ui.hGroupPosContainer, nullptr, nullptr, nullptr);

        wsprintfW(buf, L"%d", newPt.interval);
        newPt.hEditInterval = CreateWindowW(L"edit", buf,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER,
            editX, baseY, editW, rowH, m_ui.hGroupPosContainer, nullptr, nullptr, nullptr);

        newPt.hLine = UIBuilder::createSeparator(m_ui.hGroupPosContainer,
            DpiHelper::scale(8, m_currentDpi), lineY, editLeftLimitRight);

        FontManager::applyTo({ newPt.hEditCoord, newPt.hEditInterval }, m_fonts.content());
        m_clickPoints.push_back(newPt);
    }

    if (m_ui.hGroupPosContainer)
        InvalidateRect(m_ui.hGroupPosContainer, nullptr, TRUE);
}

// ---------------------------------------------------------------------------
//  saveCurrentProfile — read UI state and save as a named profile.
// ---------------------------------------------------------------------------
void MainWindow::saveCurrentProfile()
{
    wchar_t name[100];
    GetWindowTextW(m_ui.hComboProfiles, name, 100);
    if (wcslen(name) == 0)
    {
        MessageBoxW(m_hWnd, L"\x8BF7\x8F93\x5165\x914D\x7F6E\x540D\x79F0",
                    L"\x9519\x8BEF", MB_OK);  // "请输入配置名称" / "错误"
        return;
    }

    ClickSettings settings = readSettingsFromUI();

    SettingsProfile profile;
    profile.name = name;
    profile.settings = settings;

    // Update or insert.
    bool found = false;
    for (auto& p : m_profileList)
    {
        if (p.name == name)
        {
            p = profile;
            found = true;
            break;
        }
    }
    if (!found)
    {
        m_profileList.push_back(profile);
        SendMessageW(m_ui.hComboProfiles, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
    }

    m_profiles.saveAll(m_profileList);
    MessageBoxW(m_hWnd, L"\x914D\x7F6E\x5DF2\x4FDD\x5B58!",
                L"\x6210\x529F", MB_OK);  // "配置已保存!" / "成功"
}

// ---------------------------------------------------------------------------
//  deleteSelectedProfile
// ---------------------------------------------------------------------------
void MainWindow::deleteSelectedProfile()
{
    int sel = static_cast<int>(SendMessageW(m_ui.hComboProfiles, CB_GETCURSEL, 0, 0));
    if (sel == CB_ERR) return;

    wchar_t name[100];
    SendMessageW(m_ui.hComboProfiles, CB_GETLBTEXT, sel, reinterpret_cast<LPARAM>(name));

    m_profileList.erase(
        std::remove_if(m_profileList.begin(), m_profileList.end(),
            [&](const SettingsProfile& p) { return p.name == name; }),
        m_profileList.end());

    SendMessageW(m_ui.hComboProfiles, CB_DELETESTRING, sel, 0);
    m_profiles.deleteProfile(name);
    m_profiles.saveAll(m_profileList);
}

// ---------------------------------------------------------------------------
//  onPickPosition — install a low-level mouse hook to capture the next click.
// ---------------------------------------------------------------------------
void MainWindow::onPickPosition()
{
    m_pickingPos = true;
    m_hMouseHook = SetWindowsHookExW(WH_MOUSE_LL, mouseHookProc, nullptr, 0);
}

// ---------------------------------------------------------------------------
//  mouseHookProc — static hook callback for position picking.
//  Coordinate rows are created with DPI-scaled positions.
// ---------------------------------------------------------------------------
LRESULT CALLBACK MainWindow::mouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    HWND hWndMain = FindWindowW(L"AutoClickerWnd", L"AutoClicker");
    if (!hWndMain) return CallNextHookEx(nullptr, nCode, wParam, lParam);

    MainWindow* pThis = reinterpret_cast<MainWindow*>(
        GetWindowLongPtrW(hWndMain, GWLP_USERDATA));

    if (pThis && pThis->m_pickingPos && nCode == HC_ACTION && wParam == WM_LBUTTONDOWN)
    {
        auto p = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);

        ClickPoint newPoint;
        newPoint.pos = p->pt;

        // Default interval from the global "Fixed" edit.
        wchar_t buf[64];
        GetWindowTextW(pThis->m_ui.hEditCurrent, buf, 64);
        newPoint.interval = std::max(0, _wtoi(buf));

        int index = static_cast<int>(pThis->m_clickPoints.size()) + 1;

        // DPI-scaled positions.
        UINT dpi = pThis->m_currentDpi;
        int baseY = DpiHelper::scale(Theme::COORD_ROW_START_Y, dpi)
                  + (index - 1) * DpiHelper::scale(Theme::COORD_ROW_STEP, dpi);
        int labelX = DpiHelper::scale(Theme::COORD_LABEL_X, dpi);
        int labelW = DpiHelper::scale(Theme::COORD_LABEL_W, dpi);
        int rowH   = DpiHelper::scale(Theme::COORD_ROW_H, dpi);
        int editX  = DpiHelper::scale(Theme::COORD_EDIT_X, dpi);
        int editW  = DpiHelper::scale(Theme::COORD_EDIT_W, dpi);
        int lineY  = baseY + DpiHelper::scale(Theme::COORD_LINE_Y_OFFSET, dpi);
        int editLeftLimitRight = DpiHelper::scale(Theme::EDIT_LEFT_LIMIT_RIGHT, dpi);

        // Coordinate label
        wsprintfW(buf, L"%d. (%d, %d)", index, newPoint.pos.x, newPoint.pos.y);
        newPoint.hEditCoord = CreateWindowW(L"static", buf,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            labelX, baseY, labelW, rowH,
            pThis->m_ui.hGroupPosContainer, nullptr, nullptr, nullptr);

        // Interval edit
        wsprintfW(buf, L"%d", newPoint.interval);
        newPoint.hEditInterval = CreateWindowW(L"edit", buf,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER,
            editX, baseY, editW, rowH,
            pThis->m_ui.hGroupPosContainer, nullptr, nullptr, nullptr);

        // Separator
        newPoint.hLine = UIBuilder::createSeparator(pThis->m_ui.hGroupPosContainer,
            DpiHelper::scale(8, dpi), lineY, editLeftLimitRight);

        // Fonts
        FontManager::applyTo({ newPoint.hEditCoord, newPoint.hEditInterval },
                             pThis->m_fonts.content());

        pThis->m_clickPoints.push_back(newPoint);

        // Switch to "fixed position" mode.
        SendMessageW(pThis->m_ui.hRadioFixedPos, BM_SETCHECK, BST_CHECKED, 0);
        SendMessageW(pThis->m_ui.hRadioFreePos, BM_SETCHECK, BST_UNCHECKED, 0);

        pThis->m_pickingPos = false;
        UnhookWindowsHookEx(pThis->m_hMouseHook);
        pThis->m_hMouseHook = nullptr;
    }

    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

// ---------------------------------------------------------------------------
//  onClearPositions
// ---------------------------------------------------------------------------
void MainWindow::onClearPositions()
{
    UIBuilder::clearCoordRows(m_ui, m_clickPoints);
    SetWindowTextW(m_ui.hStatus, L"\x72B6\x6001: \x5750\x6807\x5DF2\x6E05\x7A7A");  // "状态: 坐标已清空"
}

// ---------------------------------------------------------------------------
//  onSetHotkey — begin capturing the next keypress as the new hotkey.
// ---------------------------------------------------------------------------
void MainWindow::onSetHotkey()
{
    m_hotkey.beginCapture(m_hWnd);
    MessageBoxW(m_hWnd,
        L"\x8BF7\x6309\x4E0B\x8981\x8BBE\x7F6E\x7684\x5FEB\x6377\x952E...",  // "请按下要设置的快捷键..."
        L"\x8BBE\x7F6E\x70ED\x952E", MB_OK);  // "设置热键"
}

// ---------------------------------------------------------------------------
//  updateHotkeyDisplay — refresh the static text showing the current hotkey.
// ---------------------------------------------------------------------------
void MainWindow::updateHotkeyDisplay()
{
    if (m_ui.hStaticHotkey)
    {
        SetWindowTextW(m_ui.hStaticHotkey,
                       HotkeyManager::vkToName(m_hotkey.getVK()));
    }
}

// ---------------------------------------------------------------------------
//  hideToTray / restoreFromTray
// ---------------------------------------------------------------------------
void MainWindow::hideToTray()
{
    ShowWindow(m_hWnd, SW_HIDE);
    m_hiddenToTray = true;
}

void MainWindow::restoreFromTray()
{
    ShowWindow(m_hWnd, SW_SHOW);
    SetForegroundWindow(m_hWnd);
    m_hiddenToTray = false;
}

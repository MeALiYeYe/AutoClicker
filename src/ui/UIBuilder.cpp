// =============================================================================
//  UIBuilder.cpp - UI layout construction for the main window.
// =============================================================================

#include "UIBuilder.h"
#include "../core/ClickPoint.h"
#include <algorithm>

// ---------------------------------------------------------------------------
//  build — create every child control and return the handle struct.
// ---------------------------------------------------------------------------
UIControls UIBuilder::build(HWND hWnd, const FontManager& fonts)
{
    UIControls ui;

    HFONT hTitle   = fonts.title();
    HFONT hContent = fonts.content();
    HFONT hSmall   = fonts.small();

    int y = Theme::MARGIN_Y;

    // =====================================================================
    //  Row 1: Click Frequency (left) + Coordinate list (right)
    // =====================================================================

    // --- Click Frequency group ---
    ui.hGroupFreq = CreateWindowW(L"button", L"\x70B9\x51FB\x9891\x7387",  // "点击频率"
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        Theme::MARGIN_X, y, Theme::HALF_WIDTH, Theme::INTERVAL_GROUP_H, hWnd, nullptr, nullptr, nullptr);
    FontManager::apply(ui.hGroupFreq, hTitle);

    int fy = y + Theme::GROUPBOX_HEADER_H + Theme::SMALL_GAP;

    // Fixed interval radio + edit
    ui.hRadioFixed = CreateWindowW(L"button", L"\x56FA\x5B9A(ms)",  // "固定(ms)"
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        Theme::MARGIN_X + Theme::SMALL_GAP, fy, Theme::RADIO_WIDTH, Theme::RADIO_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ui.hEditCurrent = CreateWindowW(L"edit", L"1500",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER,
        Theme::MARGIN_X + Theme::RADIO_WIDTH + Theme::SMALL_GAP * 2, fy,
        Theme::EDIT_WIDTH, Theme::EDIT_HEIGHT, hWnd, nullptr, nullptr, nullptr);

    // Random offset radio + edit
    int fy2 = fy + Theme::RADIO_HEIGHT + Theme::SMALL_GAP;
    ui.hRadioRandom = CreateWindowW(L"button", L"\x504F\x79FB(ms)",  // "偏移(ms)"
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        Theme::MARGIN_X + Theme::SMALL_GAP, fy2, Theme::RADIO_WIDTH, Theme::RADIO_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ui.hEditRandom = CreateWindowW(L"edit", L"500",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER,
        Theme::MARGIN_X + Theme::RADIO_WIDTH + Theme::SMALL_GAP * 2, fy2,
        Theme::EDIT_WIDTH, Theme::EDIT_HEIGHT, hWnd, nullptr, nullptr, nullptr);

    FontManager::applyTo({ ui.hRadioFixed, ui.hEditCurrent, ui.hRadioRandom, ui.hEditRandom }, hContent);
    SendMessageW(ui.hRadioRandom, BM_SETCHECK, BST_CHECKED, 0);

    // --- Coordinate list container (right side, spans both rows) ---
    int containerH = Theme::INTERVAL_GROUP_H + Theme::GROUP_GAP + Theme::POS_GROUP_H;
    ui.hGroupPosContainer = CreateWindowW(L"button",
        L"\x5750\x6807 & \x5355\x70B9\x9891\x7387(ms)",  // "坐标 & 单点频率(ms)"
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        Theme::MARGIN_X + Theme::HALF_WIDTH + Theme::SMALL_GAP, y,
        Theme::GROUP_WIDTH - Theme::HALF_WIDTH - Theme::MARGIN_X, containerH, hWnd, nullptr, nullptr, nullptr);
    FontManager::apply(ui.hGroupPosContainer, hTitle);

    y += Theme::INTERVAL_GROUP_H + Theme::GROUP_GAP;

    // =====================================================================
    //  Row 2: Click Position (left)
    // =====================================================================

    ui.hGroupPos = CreateWindowW(L"button", L"\x70B9\x51FB\x4F4D\x7F6E",  // "点击位置"
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        Theme::MARGIN_X, y, Theme::HALF_WIDTH, Theme::POS_GROUP_H, hWnd, nullptr, nullptr, nullptr);
    FontManager::apply(ui.hGroupPos, hTitle);

    int py = y + Theme::GROUPBOX_HEADER_H + Theme::SMALL_GAP;

    // Left column: 4 radios
    ui.hRadioFixedPos = CreateWindowW(L"button", L"\x5750\x6807\x4F4D\x7F6E",  // "坐标位置"
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        Theme::MARGIN_X + Theme::SMALL_GAP, py, Theme::RADIO_WIDTH, Theme::RADIO_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ui.hRadioFreePos = CreateWindowW(L"button", L"\x6307\x9488\x4F4D\x7F6E",   // "指针位置"
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        Theme::MARGIN_X + Theme::SMALL_GAP, py + Theme::RADIO_HEIGHT + Theme::SMALL_GAP,
        Theme::RADIO_WIDTH, Theme::RADIO_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ui.hRadioSeq = CreateWindowW(L"button", L"\x987A\x5E8F\x5FAA\x73AF",       // "顺序循环"
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        Theme::MARGIN_X + Theme::SMALL_GAP, py + 2 * (Theme::RADIO_HEIGHT + Theme::SMALL_GAP),
        Theme::RADIO_WIDTH, Theme::RADIO_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ui.hRadioRandPos = CreateWindowW(L"button", L"\x968F\x673A\x5FAA\x73AF",   // "随机循环"
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        Theme::MARGIN_X + Theme::SMALL_GAP, py + 3 * (Theme::RADIO_HEIGHT + Theme::SMALL_GAP),
        Theme::RADIO_WIDTH, Theme::RADIO_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    SendMessageW(ui.hRadioFixedPos, BM_SETCHECK, BST_CHECKED, 0);

    // Right column: checkboxes, radius, buttons
    int rx = Theme::MARGIN_X + Theme::RADIO_WIDTH + Theme::SMALL_GAP * 5;

    ui.hCheckRandomArea = CreateWindowW(L"button", L"\x8303\x56F4\x968F\x673A",  // "范围随机"
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        rx, py, Theme::RADIO_WIDTH, Theme::EDIT_HEIGHT, hWnd, nullptr, nullptr, nullptr);

    ui.hLblRadius = CreateWindowW(L"static", L"\x534A\x5F84:",  // "半径:"
        WS_CHILD | WS_VISIBLE,
        rx, py + Theme::RADIO_HEIGHT + Theme::SMALL_GAP,
        Theme::SHORT_LABEL_WIDTH, Theme::EDIT_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ui.hEditAreaRadius = CreateWindowW(L"edit", L"20",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER,
        rx + Theme::SHORT_LABEL_WIDTH, py + Theme::RADIO_HEIGHT + Theme::SMALL_GAP,
        Theme::SHORT_LABEL_WIDTH, Theme::EDIT_HEIGHT, hWnd, nullptr, nullptr, nullptr);

    ui.hBtnPick = CreateWindowW(L"button", L"\x70B9\x9009\x5750\x6807",  // "点选坐标"
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        rx, py + 2 * (Theme::RADIO_HEIGHT + Theme::SMALL_GAP),
        Theme::BUTTON_WIDTH, Theme::BUTTON_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ui.hBtnClearPos = CreateWindowW(L"button", L"\x6E05\x7A7A\x5750\x6807",  // "清空坐标"
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        rx, py + 3 * (Theme::RADIO_HEIGHT + Theme::SMALL_GAP),
        Theme::BUTTON_WIDTH, Theme::BUTTON_HEIGHT, hWnd, nullptr, nullptr, nullptr);

    FontManager::applyTo({
        ui.hRadioFixedPos, ui.hRadioFreePos, ui.hRadioSeq, ui.hRadioRandPos,
        ui.hCheckRandomArea, ui.hLblRadius, ui.hEditAreaRadius, ui.hBtnPick, ui.hBtnClearPos
    }, hContent);

    y += Theme::POS_GROUP_H + Theme::GROUP_GAP;

    // =====================================================================
    //  Row 3: Limit (left) + Click method (mid) + Profile (right)
    // =====================================================================

    // --- Repetition limit ---
    ui.hGroupLimit = CreateWindowW(L"button", L"\x91CD\x590D\x6B21\x6570",  // "重复次数"
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        Theme::MARGIN_X, y, Theme::HALF_WIDTH, Theme::LIMIT_GROUP_H, hWnd, nullptr, nullptr, nullptr);
    FontManager::apply(ui.hGroupLimit, hTitle);

    int ly = y + Theme::GROUPBOX_HEADER_H + Theme::SMALL_GAP;

    ui.hRadioLimitForever = CreateWindowW(L"button", L"\x65E0\x9650\x5236",  // "无限制"
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        Theme::MARGIN_X + Theme::SMALL_GAP, ly, Theme::LABEL_WIDTH, Theme::EDIT_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ly += Theme::EDIT_HEIGHT + Theme::SMALL_GAP;

    ui.hRadioLimitCount = CreateWindowW(L"button", L"\x6B21\x6570(\x6B21)",  // "次数(次)"
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        Theme::MARGIN_X + Theme::SMALL_GAP, ly, Theme::LABEL_WIDTH, Theme::EDIT_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ui.hEditTimes = CreateWindowW(L"edit", L"1000",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER,
        Theme::MARGIN_X + Theme::LABEL_WIDTH + Theme::SMALL_GAP * 2, ly,
        Theme::EDIT_WIDTH, Theme::EDIT_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ly += Theme::EDIT_HEIGHT + Theme::SMALL_GAP;

    ui.hRadioLimitTime = CreateWindowW(L"button", L"\x65F6\x957F(s)",  // "时长(s)"
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        Theme::MARGIN_X + Theme::SMALL_GAP, ly, Theme::LABEL_WIDTH, Theme::EDIT_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ui.hEditDuration = CreateWindowW(L"edit", L"3600",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER,
        Theme::MARGIN_X + Theme::LABEL_WIDTH + Theme::SMALL_GAP * 2, ly,
        Theme::EDIT_WIDTH, Theme::EDIT_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ly += Theme::EDIT_HEIGHT + Theme::SMALL_GAP;

    ui.hCheckRest = CreateWindowW(L"button", L"\x4F11\x606F(s)",  // "休息(s)"
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        Theme::MARGIN_X + Theme::SMALL_GAP, ly, Theme::LABEL_WIDTH, Theme::RADIO_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ui.hEditRestTime = CreateWindowW(L"edit", L"600",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER,
        Theme::MARGIN_X + Theme::LABEL_WIDTH + Theme::SMALL_GAP * 2, ly,
        Theme::EDIT_WIDTH, Theme::EDIT_HEIGHT, hWnd, nullptr, nullptr, nullptr);

    SendMessageW(ui.hRadioLimitForever, BM_SETCHECK, BST_CHECKED, 0);

    FontManager::applyTo({
        ui.hRadioLimitForever, ui.hRadioLimitCount, ui.hEditTimes,
        ui.hRadioLimitTime, ui.hEditDuration, ui.hCheckRest, ui.hEditRestTime
    }, hContent);

    // --- Click method (middle) ---
    int clickW = (Theme::HALF_WIDTH - Theme::SMALL_GAP) / 2;
    int clickX = Theme::MARGIN_X + Theme::HALF_WIDTH + Theme::SMALL_GAP;

    ui.hGroupClick = CreateWindowW(L"button", L"\x70B9\x51FB\x65B9\x5F0F",  // "点击方式"
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        clickX, y, clickW, Theme::LIMIT_GROUP_H, hWnd, nullptr, nullptr, nullptr);
    FontManager::apply(ui.hGroupClick, hTitle);

    int cky = y + Theme::GROUPBOX_HEADER_H + Theme::SMALL_GAP;
    int innerW = clickW - Theme::SMALL_GAP * 2;

    ui.hComboButton = CreateWindowW(L"combobox", nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        clickX + Theme::SMALL_GAP, cky, innerW, 150, hWnd, nullptr, nullptr, nullptr);
    SendMessageW(ui.hComboButton, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"\x5DE6\x952E"));   // "左键"
    SendMessageW(ui.hComboButton, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"\x53F3\x952E"));   // "右键"
    SendMessageW(ui.hComboButton, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"\x4E2D\x952E"));   // "中键"
    SendMessageW(ui.hComboButton, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"\x6EDA\x8F6E\x4E0A")); // "滚轮上"
    SendMessageW(ui.hComboButton, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"\x6EDA\x8F6E\x4E0B")); // "滚轮下"
    SendMessageW(ui.hComboButton, CB_SETCURSEL, 0, 0);

    ui.hComboClickType = CreateWindowW(L"combobox", nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        clickX + Theme::SMALL_GAP, cky + Theme::RADIO_HEIGHT + Theme::SMALL_GAP * 2,
        innerW, 120, hWnd, nullptr, nullptr, nullptr);
    SendMessageW(ui.hComboClickType, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"\x5355\x51FB"));  // "单击"
    SendMessageW(ui.hComboClickType, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"\x53CC\x51FB"));  // "双击"
    SendMessageW(ui.hComboClickType, CB_SETCURSEL, 0, 0);

    FontManager::applyTo({ ui.hComboButton, ui.hComboClickType }, hContent);

    // --- Profile (right) ---
    int profX = clickX + clickW + Theme::SMALL_GAP;
    int profW = clickW;

    ui.hGroupProfile = CreateWindowW(L"button", L"\x914D\x7F6E\x65B9\x6848",  // "配置方案"
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        profX, y, profW, Theme::LIMIT_GROUP_H, hWnd, nullptr, nullptr, nullptr);
    FontManager::apply(ui.hGroupProfile, hTitle);

    int pfy = y + Theme::GROUPBOX_HEADER_H + Theme::SMALL_GAP;
    int pfInnerW = profW - Theme::SMALL_GAP * 2;

    ui.hComboProfiles = CreateWindowW(L"combobox", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWN,
        profX + Theme::SMALL_GAP, pfy, pfInnerW, 150, hWnd, nullptr, nullptr, nullptr);

    ui.hBtnSaveProfile = CreateWindowW(L"button", L"\x4FDD\x5B58\x914D\x7F6E",  // "保存配置"
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        profX + Theme::SMALL_GAP, pfy + Theme::BUTTON_HEIGHT + Theme::SMALL_GAP * 2,
        pfInnerW, Theme::BUTTON_HEIGHT, hWnd, nullptr, nullptr, nullptr);

    ui.hBtnDeleteProfile = CreateWindowW(L"button", L"\x5220\x9664\x914D\x7F6E",  // "删除配置"
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        profX + Theme::SMALL_GAP, pfy + (Theme::BUTTON_HEIGHT + Theme::SMALL_GAP) * 2,
        pfInnerW, Theme::BUTTON_HEIGHT, hWnd, nullptr, nullptr, nullptr);

    FontManager::applyTo({ ui.hComboProfiles, ui.hBtnSaveProfile, ui.hBtnDeleteProfile }, hContent);

    y += Theme::LIMIT_GROUP_H + Theme::GROUP_GAP;

    // =====================================================================
    //  Row 4: Control bar
    // =====================================================================

    int ctrlGroupH = Theme::BUTTON_HEIGHT + Theme::GROUPBOX_HEADER_H + Theme::SMALL_GAP * 2;
    ui.hGroupCtrl = CreateWindowW(L"button", nullptr,
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        Theme::MARGIN_X, y, Theme::GROUP_WIDTH - Theme::SMALL_GAP, ctrlGroupH, hWnd, nullptr, nullptr, nullptr);
    FontManager::apply(ui.hGroupCtrl, hTitle);

    int ctrlY = y + (ctrlGroupH - Theme::BUTTON_HEIGHT) / 2 + Theme::SMALL_GAP;
    int ctrlBtnW = (Theme::GROUP_WIDTH - Theme::SMALL_GAP * 5) / 4;

    ui.hBtnStartPause = CreateWindowW(L"button", L"\x5F00\x59CB/\x6682\x505C",  // "开始/暂停"
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        Theme::MARGIN_X, ctrlY, ctrlBtnW, Theme::BUTTON_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ui.hBtnStop = CreateWindowW(L"button", L"\x505C\x6B62",  // "停止"
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        Theme::MARGIN_X + (ctrlBtnW + Theme::SMALL_GAP) * 1, ctrlY,
        ctrlBtnW, Theme::BUTTON_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ui.hBtnSetHotkey = CreateWindowW(L"button", L"\x8BBE\x7F6E\x70ED\x952E",  // "设置热键"
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        Theme::MARGIN_X + (ctrlBtnW + Theme::SMALL_GAP) * 2, ctrlY,
        ctrlBtnW, Theme::BUTTON_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ui.hStaticHotkey = CreateWindowW(L"static", L"F6",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE | WS_BORDER,
        Theme::MARGIN_X + (ctrlBtnW + Theme::SMALL_GAP) * 3, ctrlY,
        ctrlBtnW, Theme::BUTTON_HEIGHT, hWnd, nullptr, nullptr, nullptr);

    FontManager::applyTo({ ui.hBtnStartPause, ui.hBtnStop, ui.hBtnSetHotkey, ui.hStaticHotkey }, hContent);

    y += ctrlGroupH + Theme::GROUP_GAP;

    // =====================================================================
    //  Row 5: Status bar
    // =====================================================================

    ui.hStatus = CreateWindowW(L"static",
        L"\x72B6\x6001: \x672A\x542F\x52A8",  // "状态: 未启动"
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        Theme::MARGIN_X + Theme::SMALL_GAP, y, Theme::TYPE_GROUP_W, Theme::STATUS_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ui.hStatusInterval = CreateWindowW(L"static",
        L"\x95F4\x9694: -",  // "间隔: -"
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        Theme::MARGIN_X + Theme::TYPE_GROUP_W + Theme::SMALL_GAP * 2, y,
        Theme::TYPE_GROUP_W, Theme::STATUS_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ui.hStatusCountTime = CreateWindowW(L"static",
        L"\x5269\x4F59: -",  // "剩余: -"
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        Theme::MARGIN_X + Theme::TYPE_GROUP_W * 2 + Theme::SMALL_GAP * 3, y,
        Theme::TYPE_GROUP_W, Theme::STATUS_HEIGHT, hWnd, nullptr, nullptr, nullptr);

    FontManager::applyTo({ ui.hStatus, ui.hStatusInterval, ui.hStatusCountTime }, hSmall);

    y += Theme::STATUS_HEIGHT + Theme::GROUP_GAP;

    // =====================================================================
    //  Row 6: Progress bar
    // =====================================================================

    ui.hLblProgress = CreateWindowW(L"static",
        L"\x8FDB\x5EA6:",  // "进度:"
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        Theme::MARGIN_X + Theme::SMALL_GAP, y, Theme::LABEL_WIDTH, Theme::BUTTON_HEIGHT, hWnd, nullptr, nullptr, nullptr);
    ui.hProgress = CreateWindowW(L"static", nullptr,
        WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
        Theme::MARGIN_X + Theme::LABEL_WIDTH + Theme::SMALL_GAP, y,
        Theme::GROUP_WIDTH - Theme::LABEL_WIDTH - Theme::SMALL_GAP * 2, Theme::BUTTON_HEIGHT,
        hWnd, reinterpret_cast<HMENU>(1001), GetModuleHandleW(nullptr), nullptr);

    FontManager::apply(ui.hLblProgress, hContent);

    return ui;
}

// ---------------------------------------------------------------------------
//  createSeparator
// ---------------------------------------------------------------------------
HWND UIBuilder::createSeparator(HWND parent, int x, int y, int width)
{
    return CreateWindowW(L"static", nullptr,
        WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        x, y, width, 2, parent, nullptr, nullptr, nullptr);
}

// ---------------------------------------------------------------------------
//  createCoordRow — add a labeled coordinate row to the container.
// ---------------------------------------------------------------------------
void UIBuilder::createCoordRow(UIControls& ui, int index, const ClickPoint& pt,
                               const FontManager& fonts)
{
    int baseY = 25 + (index - 1) * 30;
    wchar_t buf[128];

    // Label: "N. (x, y)"
    wsprintfW(buf, L"%d. (%d, %d)", index, pt.pos.x, pt.pos.y);
    HWND hCoord = CreateWindowW(L"static", buf,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, baseY, 100, 22, ui.hGroupPosContainer, nullptr, nullptr, nullptr);

    // Interval edit box
    wsprintfW(buf, L"%d", pt.interval);
    HWND hInterval = CreateWindowW(L"edit", buf,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER,
        130, baseY, 50, 22, ui.hGroupPosContainer, nullptr, nullptr, nullptr);

    // Separator line
    HWND hLine = createSeparator(ui.hGroupPosContainer, 8, baseY + 24,
        Theme::EDIT_LEFT_LIMIT_RIGHT);

    FontManager::applyTo({ hCoord, hInterval }, fonts.content());
}

// ---------------------------------------------------------------------------
//  clearCoordRows — destroy coordinate row child windows and clear vector.
// ---------------------------------------------------------------------------
void UIBuilder::clearCoordRows(UIControls& ui, std::vector<ClickPoint>& clickPoints)
{
    for (const auto& pt : clickPoints)
    {
        if (pt.hEditCoord)    DestroyWindow(pt.hEditCoord);
        if (pt.hEditInterval) DestroyWindow(pt.hEditInterval);
        if (pt.hLine)         DestroyWindow(pt.hLine);
    }
    clickPoints.clear();

    // Reset to "pointer position" mode.
    if (ui.hRadioFreePos)  SendMessageW(ui.hRadioFreePos, BM_SETCHECK, BST_CHECKED, 0);
    if (ui.hRadioFixedPos) SendMessageW(ui.hRadioFixedPos, BM_SETCHECK, BST_UNCHECKED, 0);

    if (ui.hGroupPosContainer)
        InvalidateRect(ui.hGroupPosContainer, nullptr, TRUE);
}

#pragma once
// =============================================================================
//  ClickStrategies.h - Concrete click strategy implementations
//  Demonstrates INHERITANCE: every class below derives from ClickStrategy
//  and overrides execute() with type-specific SendInput calls.
//
//  Click mode mapping (matches original Onmyoji clickMode values):
//    0 = Left single    1 = Right single     2 = Left double
//    3 = Right double   4 = Wheel up         5 = Wheel down
//    6 = Middle single
// =============================================================================

#include "ClickStrategy.h"

// ---------------------------------------------------------------------------
//  LeftClickStrategy — single left-click (clickMode 0)
// ---------------------------------------------------------------------------
class LeftClickStrategy : public ClickStrategy
{
public:
    void execute(POINT pos) const override
    {
        if (pos.x >= 0 && pos.y >= 0)
            SetCursorPos(pos.x, pos.y);

        INPUT inputs[2] = {};
        inputs[0].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        inputs[1].type = INPUT_MOUSE;
        inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(2, inputs, sizeof(INPUT));
    }
    const wchar_t* name() const override { return L"Left Click"; }
};

// ---------------------------------------------------------------------------
//  RightClickStrategy — single right-click (clickMode 1)
// ---------------------------------------------------------------------------
class RightClickStrategy : public ClickStrategy
{
public:
    void execute(POINT pos) const override
    {
        if (pos.x >= 0 && pos.y >= 0)
            SetCursorPos(pos.x, pos.y);

        INPUT inputs[2] = {};
        inputs[0].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        inputs[1].type = INPUT_MOUSE;
        inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        SendInput(2, inputs, sizeof(INPUT));
    }
    const wchar_t* name() const override { return L"Right Click"; }
};

// ---------------------------------------------------------------------------
//  DoubleLeftClickStrategy — double left-click (clickMode 2)
// ---------------------------------------------------------------------------
class DoubleLeftClickStrategy : public ClickStrategy
{
public:
    void execute(POINT pos) const override
    {
        if (pos.x >= 0 && pos.y >= 0)
            SetCursorPos(pos.x, pos.y);

        INPUT inputs[2] = {};
        inputs[0].type = INPUT_MOUSE;
        inputs[1].type = INPUT_MOUSE;

        for (int i = 0; i < 2; ++i)
        {
            inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
            SendInput(2, inputs, sizeof(INPUT));
            Sleep(40);
        }
    }
    const wchar_t* name() const override { return L"Double Left Click"; }
};

// ---------------------------------------------------------------------------
//  DoubleRightClickStrategy — double right-click (clickMode 3)
// ---------------------------------------------------------------------------
class DoubleRightClickStrategy : public ClickStrategy
{
public:
    void execute(POINT pos) const override
    {
        if (pos.x >= 0 && pos.y >= 0)
            SetCursorPos(pos.x, pos.y);

        INPUT inputs[2] = {};
        inputs[0].type = INPUT_MOUSE;
        inputs[1].type = INPUT_MOUSE;

        for (int i = 0; i < 2; ++i)
        {
            inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
            inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            SendInput(2, inputs, sizeof(INPUT));
            Sleep(40);
        }
    }
    const wchar_t* name() const override { return L"Double Right Click"; }
};

// ---------------------------------------------------------------------------
//  WheelUpStrategy — mouse wheel scroll up (clickMode 4)
// ---------------------------------------------------------------------------
class WheelUpStrategy : public ClickStrategy
{
public:
    void execute(POINT pos) const override
    {
        if (pos.x >= 0 && pos.y >= 0)
            SetCursorPos(pos.x, pos.y);

        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        input.mi.mouseData = WHEEL_DELTA;
        SendInput(1, &input, sizeof(INPUT));
    }
    const wchar_t* name() const override { return L"Wheel Up"; }
};

// ---------------------------------------------------------------------------
//  WheelDownStrategy — mouse wheel scroll down (clickMode 5)
// ---------------------------------------------------------------------------
class WheelDownStrategy : public ClickStrategy
{
public:
    void execute(POINT pos) const override
    {
        if (pos.x >= 0 && pos.y >= 0)
            SetCursorPos(pos.x, pos.y);

        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        input.mi.mouseData = -WHEEL_DELTA;
        SendInput(1, &input, sizeof(INPUT));
    }
    const wchar_t* name() const override { return L"Wheel Down"; }
};

// ---------------------------------------------------------------------------
//  MiddleClickStrategy — single middle-click (clickMode 6)
// ---------------------------------------------------------------------------
class MiddleClickStrategy : public ClickStrategy
{
public:
    void execute(POINT pos) const override
    {
        if (pos.x >= 0 && pos.y >= 0)
            SetCursorPos(pos.x, pos.y);

        INPUT inputs[2] = {};
        inputs[0].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
        inputs[1].type = INPUT_MOUSE;
        inputs[1].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
        SendInput(2, inputs, sizeof(INPUT));
    }
    const wchar_t* name() const override { return L"Middle Click"; }
};

// ---------------------------------------------------------------------------
//  StrategyFactory — creates the correct strategy from a clickMode integer.
//  Returns a heap-allocated pointer; caller owns it (use unique_ptr).
// ---------------------------------------------------------------------------
#include <memory>

inline std::unique_ptr<ClickStrategy> createClickStrategy(DWORD clickMode)
{
    switch (clickMode)
    {
    case 0: return std::make_unique<LeftClickStrategy>();
    case 1: return std::make_unique<RightClickStrategy>();
    case 2: return std::make_unique<DoubleLeftClickStrategy>();
    case 3: return std::make_unique<DoubleRightClickStrategy>();
    case 4: return std::make_unique<WheelUpStrategy>();
    case 5: return std::make_unique<WheelDownStrategy>();
    case 6: return std::make_unique<MiddleClickStrategy>();
    default: return std::make_unique<LeftClickStrategy>();
    }
}

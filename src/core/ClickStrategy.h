#pragma once
// =============================================================================
//  ClickStrategy.h - Abstract base class for mouse click strategies
//  Demonstrates POLYMORPHISM: all click types share a common interface,
//  allowing ClickEngine to treat every click type uniformly via a base pointer.
// =============================================================================

#include <windows.h>

class ClickStrategy
{
public:
    virtual ~ClickStrategy() = default;

    // Pure virtual: each derived strategy implements its own click behavior.
    // pos = target screen coordinates (already adjusted for area-random offset).
    virtual void execute(POINT pos) const = 0;

    // Optional: human-readable name for debugging / logging.
    virtual const wchar_t* name() const { return L"Unknown"; }
};

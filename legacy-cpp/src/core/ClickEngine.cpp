// =============================================================================
//  ClickEngine.cpp - Implementation of the background click engine.
// =============================================================================

#include "ClickEngine.h"
#include "ClickStrategies.h"
#include <random>
#include <algorithm>

ClickEngine::ClickEngine()
{
    // Pre-start the worker thread in a idle loop; it waits for running=true.
    stopFlag = false;
    worker = std::thread(&ClickEngine::workerThread, this, ClickSettings{});
}

ClickEngine::~ClickEngine()
{
    stopFlag = true;
    running  = false;
    if (worker.joinable())
        worker.join();
}

// ---------------------------------------------------------------------------
//  start — begin clicking with the given settings.
// ---------------------------------------------------------------------------
void ClickEngine::start(const ClickSettings& settings)
{
    if (running.load())
        return;

    // Restart the worker with fresh settings.
    stopFlag = true;
    running  = false;
    if (worker.joinable())
        worker.join();

    stopFlag = false;
    running  = true;
    worker   = std::thread(&ClickEngine::workerThread, this, settings);
}

// ---------------------------------------------------------------------------
//  pause — stop clicking but keep the engine alive (can restart later).
// ---------------------------------------------------------------------------
void ClickEngine::pause()
{
    running = false;
    notifyStatus(L"\x72B6\x6001: \x5DF2\x6682\x505C");  // "状态: 已暂停"
}

// ---------------------------------------------------------------------------
//  stop — fully stop and reset counters.
// ---------------------------------------------------------------------------
void ClickEngine::stop()
{
    running = false;
    progressPercent = 0;
    notifyStatus(L"\x72B6\x6001: \x5DF2\x505C\x6B62");  // "状态: 已停止"
    notifyInterval(0);
    notifyProgress(0);
}

// ---------------------------------------------------------------------------
//  workerThread — the main scheduling loop (runs in a dedicated thread).
// ---------------------------------------------------------------------------
void ClickEngine::workerThread(ClickSettings settings)
{
    cycleCount  = 0;
    totalCount  = 0;
    cycleStart  = std::chrono::steady_clock::now();
    taskStart   = cycleStart;

    // Create the polymorphic click strategy from settings.
    auto strategy = createClickStrategy(settings.clickMode);

    // RNG for random intervals and area jitter.
    std::random_device rd;
    std::mt19937 gen(rd());

    // Initialize per-point scheduling times.
    auto now = std::chrono::steady_clock::now();
    for (auto& p : settings.clickPoints)
    {
        int base = (p.interval > 0) ? p.interval : settings.intervalFixed;
        p.nextTime = now + std::chrono::milliseconds(computeInterval(base, settings));
    }

    notifyStatus(L"\x72B6\x6001: \x6B63\x5728\x70B9\x51FB...");  // "状态: 正在点击..."

    while (!stopFlag.load())
    {
        if (!running.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        auto tickNow = std::chrono::steady_clock::now();
        bool clickedThisLoop = false;

        // --- Multi-point mode: iterate all configured points ---
        if (!settings.clickPoints.empty() && settings.clickPosFixed)
        {
            // Optionally shuffle point order for random-position mode.
            if (settings.useRandomPos)
            {
                // We don't shuffle the vector itself (it has UI handles);
                // instead we track which points are due and pick randomly.
            }

            for (auto& p : settings.clickPoints)
            {
                if (tickNow >= p.nextTime)
                {
                    POINT target = p.pos;
                    if (settings.useAreaRandom && settings.areaRadius > 0)
                        target = applyAreaJitter(target, settings.areaRadius);

                    strategy->execute(target);
                    clickedThisLoop = true;
                    cycleCount++;
                    totalCount++;

                    int base = (p.interval > 0) ? p.interval : settings.intervalFixed;
                    int nextMs = computeInterval(base, settings);
                    p.nextTime = tickNow + std::chrono::milliseconds(nextMs);

                    notifyInterval(nextMs);
                }
            }
        }
        // --- Free-cursor mode: click at current mouse position ---
        else if (!settings.clickPosFixed)
        {
            POINT target;
            GetCursorPos(&target);
            strategy->execute(target);

            cycleCount++;
            totalCount++;

            int nextMs = computeInterval(settings.intervalFixed, settings);
            notifyInterval(nextMs);

            std::this_thread::sleep_for(std::chrono::milliseconds(nextMs));
            // Skip the rest of the loop body — we already slept.
            goto updateProgress;
        }

        // --- Rest logic (anti-addiction) ---
        if (settings.enableRest && settings.restTime > 0)
        {
            bool shouldRest = false;
            if (settings.limitMode == LimitMode::ByCount && settings.clickCountLimit > 0)
            {
                if (cycleCount >= settings.clickCountLimit)
                    shouldRest = true;
            }
            else if (settings.limitMode == LimitMode::ByTime && settings.clickDurationSec > 0)
            {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    tickNow - cycleStart).count();
                if (elapsed >= settings.clickDurationSec)
                    shouldRest = true;
            }

            if (shouldRest)
            {
                notifyStatus(L"\x72B6\x6001: \x4F11\x606F\x4E2D...");  // "状态: 休息中..."
                std::this_thread::sleep_for(std::chrono::seconds(settings.restTime));
                notifyStatus(L"\x72B6\x6001: \x6B63\x5728\x70B9\x51FB...");  // "状态: 正在点击..."
                cycleCount = 0;
                cycleStart = std::chrono::steady_clock::now();
            }
        }

    updateProgress:
        // --- Progress bar update ---
        {
            int percent = 0;
            wchar_t buf[128];

            if (settings.limitMode == LimitMode::ByCount)
            {
                int remaining = std::max(0, settings.clickCountLimit - cycleCount);
                wsprintfW(buf, L"\x5FAA\x73AF\x5269\x4F59: %d \x6B21", remaining);  // "循环剩余: N 次"
                if (settings.clickCountLimit > 0)
                    percent = static_cast<int>((cycleCount * 100.0) / settings.clickCountLimit);
            }
            else if (settings.limitMode == LimitMode::ByTime)
            {
                int elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - cycleStart).count());
                int remaining = std::max(0, settings.clickDurationSec - elapsed);
                wsprintfW(buf, L"\x5FAA\x73AF\x5269\x4F59: %d s", remaining);  // "循环剩余: N s"
                if (settings.clickDurationSec > 0)
                    percent = static_cast<int>((elapsed * 100.0) / settings.clickDurationSec);
            }
            else
            {
                wcscpy_s(buf, L"\x5FAA\x73AF: \x65E0\x9650");  // "循环: 无限"
                percent = 0;
            }

            notifyCountTime(buf);
            if (running.load())
            {
                if (percent > 100) percent = 100;
                progressPercent = percent;
                notifyProgress(percent);
            }
        }

        // High-frequency scheduling tick (1 ms for responsive multi-point).
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// ---------------------------------------------------------------------------
//  computeInterval — returns the next click interval based on settings.
// ---------------------------------------------------------------------------
int ClickEngine::computeInterval(int base, const ClickSettings& s) const
{
    if (s.useRandom)
    {
        int low  = std::max(1, base - s.intervalOffset);
        int high = base + s.intervalOffset;
        if (low > high) high = low;

        static thread_local std::mt19937 rng(
            static_cast<unsigned>(std::chrono::high_resolution_clock::now()
                .time_since_epoch().count()));
        std::uniform_int_distribution<int> dist(low, high);
        return dist(rng);
    }
    return base;
}

// ---------------------------------------------------------------------------
//  applyAreaJitter — offsets a point randomly within a circular radius.
// ---------------------------------------------------------------------------
POINT ClickEngine::applyAreaJitter(POINT pos, int radius) const
{
    static thread_local std::mt19937 rng(
        static_cast<unsigned>(std::chrono::high_resolution_clock::now()
            .time_since_epoch().count()));

    std::uniform_real_distribution<double> ang(0.0, 2.0 * 3.14159265358979323846);
    std::uniform_real_distribution<double> rad(0.0, 1.0);

    double r = radius * std::sqrt(rad(rng));
    double a = ang(rng);

    POINT result;
    result.x = pos.x + static_cast<int>(std::round(r * std::cos(a)));
    result.y = pos.y + static_cast<int>(std::round(r * std::sin(a)));
    return result;
}

// ---------------------------------------------------------------------------
//  Notification helpers — post messages to the UI window.
// ---------------------------------------------------------------------------
void ClickEngine::notifyStatus(const wchar_t* msg)
{
    if (hNotifyWnd)
        PostMessage(hNotifyWnd, WM_ENGINE_STATUS, 0, reinterpret_cast<LPARAM>(msg));
}

void ClickEngine::notifyInterval(int ms)
{
    if (hNotifyWnd)
        PostMessage(hNotifyWnd, WM_ENGINE_INTERVAL, static_cast<WPARAM>(ms), 0);
}

void ClickEngine::notifyCountTime(const wchar_t* msg)
{
    if (hNotifyWnd)
        PostMessage(hNotifyWnd, WM_ENGINE_COUNTTIME, 0, reinterpret_cast<LPARAM>(msg));
}

void ClickEngine::notifyProgress(int percent)
{
    if (hNotifyWnd)
        PostMessage(hNotifyWnd, WM_ENGINE_PROGRESS, static_cast<WPARAM>(percent), 0);
}

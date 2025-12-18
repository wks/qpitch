#pragma once

#include <chrono>

class FPSProfiler
{
public:
    FPSProfiler(const char *title, bool printLog = false);
    void tick();
    double getFPS();

private:
    using ClockType = std::chrono::steady_clock;
    using TimePointType = std::chrono::time_point<ClockType>;

    bool printLog;
    const char *title;
    bool started = false;
    TimePointType startTime;
    TimePointType lastTime;
    uint64_t tickCount = 0;
    double curFPS = 0.0;
};

#pragma once

#include <chrono>

class FPSProfiler
{
public:
    FPSProfiler(const char *title, bool printLog = false);
    void tick();
    double get_fps();

private:
    using ClockType = std::chrono::steady_clock;
    using TimePointType = std::chrono::time_point<ClockType>;

    bool printLog;
    const char *title;
    bool started = false;
    TimePointType start_time;
    TimePointType last_time;
    uint64_t tick_count = 0;
    double cur_fps = 0.0;
};

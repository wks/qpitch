#include "fpsprofiler.h"

#include <print>

FPSProfiler::FPSProfiler(const char *title): title(title) {}

void FPSProfiler::tick() {
    TimePointType now = ClockType::now();

    if (!started) {
    	started = true;
		start_time = now;
	} else {
		tick_count++;
		double elapsed_s = std::chrono::duration<double>(now - start_time).count();
		double fps = tick_count / elapsed_s;
        cur_fps = fps;
        std::println("[{}] {} events / {} s.  fps: {}", title, tick_count, elapsed_s, fps);
	}
}

double FPSProfiler::get_fps() {
    return cur_fps;
}

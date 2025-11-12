#include "fpsprofiler.h"

#include <QString>
#include <QDebug>
#include <QtLogging>

FPSProfiler::FPSProfiler(const char *title, bool printLog): title(title), printLog(printLog) {}

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
		if (printLog) {
			qDebug() << QString("[%1] %2 events / %3 s.  fps: %4").arg(title).arg(tick_count).arg(elapsed_s).arg(fps);
		}
	}
}

double FPSProfiler::get_fps() {
    return cur_fps;
}

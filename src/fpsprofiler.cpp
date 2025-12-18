#include "fpsprofiler.h"

#include <QString>
#include <QDebug>
#include <QtLogging>

FPSProfiler::FPSProfiler(const char *title, bool printLog) : title(title), printLog(printLog) { }

void FPSProfiler::tick()
{
    TimePointType now = ClockType::now();

    if (!started) {
        started = true;
        start_time = now;
    } else {
        tick_count++;
        double elapsed_since_last_s = std::chrono::duration<double>(now - last_time).count();
        double elapsed_since_start_s = std::chrono::duration<double>(now - start_time).count();
        double fps = tick_count / elapsed_since_start_s;
        cur_fps = fps;
        if (printLog) {
            qDebug().noquote()
                    << QString("[%1] FPS profiling: %2 events / %3 s.  fps: %4  since last: %5 s")
                               .arg(title)
                               .arg(tick_count)
                               .arg(elapsed_since_start_s)
                               .arg(fps)
                               .arg(elapsed_since_last_s);
        }
    }
    last_time = now;
}

double FPSProfiler::get_fps()
{
    return cur_fps;
}

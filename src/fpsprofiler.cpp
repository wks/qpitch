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
        startTime = now;
    } else {
        tickCount++;
        double elapsedSinceLastS = std::chrono::duration<double>(now - lastTime).count();
        double elapsedSinceStartS = std::chrono::duration<double>(now - startTime).count();
        double fps = tickCount / elapsedSinceStartS;
        curFPS = fps;
        if (printLog) {
            qDebug().noquote()
                    << QString("[%1] FPS profiling: %2 events / %3 s.  fps: %4  since last: %5 s")
                               .arg(title)
                               .arg(tickCount)
                               .arg(elapsedSinceStartS)
                               .arg(fps)
                               .arg(elapsedSinceLastS);
        }
    }
    lastTime = now;
}

double FPSProfiler::getFPS()
{
    return curFPS;
}

#include "visualization_data.h"

VisualizationData::VisualizationData(unsigned int plotData_size):
    plotData_size(plotData_size),
    plotSample(plotData_size),
    plotAutoCorr(plotData_size),
    timeRangeSample(0.0),
    estimatedFrequency(0.0)
{
}

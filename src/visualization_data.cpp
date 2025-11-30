#include "visualization_data.h"

VisualizationData::VisualizationData(size_t plotData_size):
    plotData_size(plotData_size),
    plotSample(plotData_size),
    plotSpectrum(plotData_size),
    plotAutoCorr(plotData_size),
    timeRangeSample(0.0),
    estimatedFrequency(0.0)
{
}

void VisualizationData::popluateSamples(double *srcSamples, size_t srcNumSamples, uint32_t sampleFrequency) {
    // Bail out for extreme cases.
    if (srcNumSamples == 0) {
        std::fill(plotSample.begin(), plotSample.end(), 0.0);
        return;
    }

    // Find the first rising edge that crosses the zero point.
    double lastSample = srcSamples[0];
    std::optional<size_t> risingEdgeIndex;
    size_t plotSampleCursor = 0;
    for (size_t k = 1; k < srcNumSamples; k++) {
        double currentSample = srcSamples[k];
        if (lastSample < 0.0 && currentSample >= 0.0) {
            risingEdgeIndex = k;
            break;
        }
        lastSample = currentSample;
    }

    size_t copyFrom = risingEdgeIndex.value_or(0);
    size_t availableSamples = srcNumSamples - copyFrom;
    size_t copyLen = std::min(plotData_size, availableSamples);

    // We don't do down-sampling.  Our oscilator view shows the actual samples.
    size_t dstCursor = 0;
    for (size_t i = copyFrom; i < copyFrom + copyLen; i++) {
        plotSample[dstCursor] = srcSamples[i];
        dstCursor++;
    }

    if (dstCursor < plotData_size) {
        plotSample[dstCursor] = 0.0;
        dstCursor++;
    }

    timeRangeSample = 1000.0 * plotData_size / sampleFrequency;
}

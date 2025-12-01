#include "visualization_data.h"

#include <algorithm>

VisualizationData::VisualizationData(size_t plotData_size):
    plotData_size(plotData_size),
    plotSample(plotData_size),
    plotSpectrum(plotData_size),
    plotAutoCorr(plotData_size),
    plotSampleRange(0.0),
    estimatedFrequency(0.0)
{
}

void VisualizationData::popluateSamples(float *srcSamples, size_t srcNumSamples, uint32_t sampleFrequency) {
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

    while (dstCursor < plotData_size) {
        plotSample[dstCursor] = 0.0;
        dstCursor++;
    }

    plotSampleRange = 1000.0 * plotData_size / sampleFrequency;
}
void VisualizationData::popluateSpectrum(fftw_complex *freqDomain, size_t srcSize, uint32_t sampleFrequency) {
    size_t available = srcSize / 2;
    size_t copyLen = std::min(plotData_size, available);

    for (size_t i = 0; i < copyLen; i++) {
        plotSpectrum[i] = freqDomain[i][0];
    }

    if (copyLen < plotData_size) {
        std::fill_n(&plotSpectrum[copyLen], plotData_size - copyLen, 0.0);
    }

    plotSpectrumRange = (double)sampleFrequency * plotData_size / (2.0 * srcSize);
}

void VisualizationData::popluateAutoCorr(double *timeDomain, size_t srcSize, uint32_t sampleFrequency, size_t multiplier) {
    size_t available = srcSize / multiplier;
    size_t copyLen = std::min(plotData_size, available);

    for (size_t i = 0; i < copyLen; i++) {
        plotAutoCorr[i] = timeDomain[i * multiplier];
    }

    if (copyLen < plotData_size) {
        std::fill_n(&plotSpectrum[copyLen], plotData_size - copyLen, 0.0);
    }

    plotAutoCorrRange = 1000.0 * plotData_size / sampleFrequency;
}

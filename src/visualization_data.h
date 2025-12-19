#pragma once

#include "notes.h"

#include <QMutex>
#include <cstdint>
#include <vector>

#include "fftw3.h"

/// A data structure that contains buffers used for visualization.
class VisualizationData
{
public:
    VisualizationData(size_t plotData_size);

    /// Try to obtain enough samples from a source to populate the plotSample array.
    void popluateSamples(float *srcSamples, size_t srcNumSamples, uint32_t sampleFrequency);
    void popluateSpectrum(fftw_complex *freqDomain, size_t srcSize, uint32_t sampleFrequency);
    void popluateAutoCorr(double *timeDomain, size_t srcSize, uint32_t sampleFrequency,
                          size_t multiplier);

    /// Ensure the QPitchCore thread doesn't write to the buffers when the UI thread is drawing
    QMutex mutex;

    /// Total number of samples used for visualization
    size_t plotData_size;

    /// Buffer used to store time samples used for visualization
    std::vector<double> plotSample;

    /// The time range of the plotSample array, in milliseconds
    double plotSampleRange;

    /// Buffer used to store frequency spectrum used for visualization
    std::vector<double> plotSpectrum;

    /// The frequency range of the plotSpectrum array, in Hz
    double plotSpectrumRange;

    /// Buffer used to store autocorrelation samples used for visualization
    std::vector<double> plotAutoCorr;

    /// The time range of the plotAutoCorr array, in milliseconds
    double plotAutoCorrRange;

    /// Estimated frequency, in Hz
    double estimatedFrequency;

    /// Estimated note
    std::optional<EstimatedNote> estimatedNote;
};

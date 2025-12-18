#pragma once

#include "notes.h"

#include <QMutex>
#include <cstdint>
#include <vector>

#include "fftw3.h"

// ** TEMPORARY BUFFERS USED FOR VISUALIZATION ** //
class VisualizationData
{
public:
    VisualizationData(size_t plotData_size);

    //! Try to obtain enough samples from a source to populate the plotSample array.
    void popluateSamples(float *srcSamples, size_t srcNumSamples, uint32_t sampleFrequency);
    void popluateSpectrum(fftw_complex *freqDomain, size_t srcSize, uint32_t sampleFrequency);
    void popluateAutoCorr(double *timeDomain, size_t srcSize, uint32_t sampleFrequency,
                          size_t multiplier);

    QMutex mutex; //!< Ensure the QPitchCore thread doesn't write to the buffers when the UI thread is drawing
    size_t plotData_size; //!< Total number of samples used for visualization
    std::vector<double> plotSample; //!< Buffer used to store time samples used for visualization
    double plotSampleRange; //!< The time range of the plotSample array, in milliseconds
    std::vector<double>
            plotSpectrum; //!< Buffer used to store frequency spectrum used for visualization
    double plotSpectrumRange; //!< The frequency range of the plotSpectrum array, in Hz
    std::vector<double>
            plotAutoCorr; //!< Buffer used to store autocorrelation samples used for visualization
    double plotAutoCorrRange; //!< The time range of the plotAutoCorr array, in milliseconds
    double estimatedFrequency; //!< Estimated frequency, in Hz
    std::optional<EstimatedNote> estimatedNote; //!< Estimated note
};

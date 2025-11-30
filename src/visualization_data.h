#pragma once

#include "notes.h"

#include <QMutex>
#include <cstdint>
#include <vector>

// ** TEMPORARY BUFFERS USED FOR VISUALIZATION ** //
class VisualizationData {
public:
    VisualizationData(size_t plotData_size);

    //! Try to obtain enough samples from a source to populate the plotSample array.
    void popluateSamples(double *srcSamples, size_t srcNumSamples, uint32_t sampleFrequency);

    QMutex mutex;                               //!< Ensure the QPitchCore thread doesn't write to the buffers when the UI thread is drawing
    size_t              plotData_size;          //!< Total number of samples used for visualization
    std::vector<double> plotSample;             //!< Buffer used to store time samples used for visualization
    std::vector<double> plotSpectrum;           //!< Buffer used to store frequency spectrum used for visualization
    std::vector<double> plotAutoCorr;           //!< Buffer used to store autocorrelation samples used for visualization
    double              timeRangeSample;        //!< The time range of the plotSample array, in milliseconds
    double              estimatedFrequency;     //!< Estimated frequency
    std::optional<EstimatedNote> estimatedNote;          //!< Estimated note
};

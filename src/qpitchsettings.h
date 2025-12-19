#pragma once

#include "notes.h"

/// Structure holding the application settings
struct QPitchSettings
{
    /// Current sample rate
    unsigned int sampleFrequency;

    /// Current size of the buffer used to compute the FFT
    unsigned int fftFrameSize;

    /// The reference frequency of A4 used to estimate the pitch
    double fundamentalFrequency;

    /// Current tuning notation
    TuningNotation tuningNotation;

    // ** METHODS ** //

    /// Default constructor.  Use default values.
    QPitchSettings();

    /// Load the settings from default values.
    void loadDefault();

    /// Load the settings from QSettings.  Only apply valid entries.
    void load();

    /// Store the settings to QSettings.
    void store();
};

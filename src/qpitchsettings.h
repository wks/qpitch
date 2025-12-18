#pragma once

#include "notes.h"

//! Structure holding the application settings
struct QPitchSettings
{
    unsigned int sampleFrequency; //!< Current sample rate
    unsigned int fftFrameSize; //!< Current size of the buffer used to compute the FFT
    double fundamentalFrequency; //!< The reference frequency of A4 used to estimate the pitch
    TuningNotation tuningNotation; //!< Current tuning notation

    // ** METHODS ** //
    QPitchSettings(); //!< Default constructor.  Use default values.
    void loadDefault(); //!< Load the settings from default values.
    void load(); //!< Load the settings from QSettings.  Only apply valid entries.
    void store(); //!< Store the settings to QSettings.
};

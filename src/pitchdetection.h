#pragma once

#include <cstdint>
#include <fftw3.h>

class PitchDetectionContext {
public: // ** CONSTANTS ** //
    static const int    ZERO_PADDING_FACTOR;                    //!< Number of times that the FFT is zero-padded to increase frequency resolution

public: // ** PUBLIC METHODS ** //
    PitchDetectionContext(uint32_t sampleFrequency, size_t fftFrameSize);
    virtual ~PitchDetectionContext();

    double* getInputBuffer();
    fftw_complex* getFreq2Buffer();
    double* getAutoCorrBuffer();

    //! Estimate the pitch of the input signal finding the first peak of the autocorrelation.
    /*!
     * \return the frequency value corresponding to the maximum of the autocorrelation
     */
    double runPitchDetectionAlgorithm();

private:
    // ** PITCH DETECTION PARAMETERS ** //
    double              _sampleFrequency;                       //!< PortAudio stream

    // ** FFTW STRUCTURES ** //
    fftw_plan           _fftw_plan_FFT;                         //!< Plan to compute the FFT of a given signal
    fftw_plan           _fftw_plan_IFFT;                        //!< Plan to compute the IFFT of a given signal (with additional zero-padding
    size_t              _fftFrameSize;                          //!< Number of frames in the time-domain input
    double*             _fftw_in_time;                          //!< External buffer used to store the input signal in the time domain
    fftw_complex*       _fftw_mid_freq;                         //!< Buffer used to store the intermediate signal in the frequency domain
    fftw_complex*       _fftw_mid_freq2;                        //!< Buffer used to store the intermediate signal in the frequency domain for auto-correlation
    double*             _fftw_out_time_autocorr;                //!< Buffer used to store the output signal in the time domain for the auto-correlation
};

#pragma once

#include <cstdint>
#include <fftw3.h>

class PitchDetectionContext {
public: // ** CONSTANTS ** //
	static const int	ZERO_PADDING_FACTOR;					//!< Number of times that the FFT is zero-padded to increase frequency resolution

public: // ** PUBLIC METHODS ** //
    PitchDetectionContext(uint32_t sampleFrequency, size_t fftFrameSize);
    virtual ~PitchDetectionContext();

	double* getInputBuffer();

	//! Estimate the pitch of the input signal finding the first peak of the autocorrelation.
	/*!
	 * \return the frequency value corresponding to the maximum of the autocorrelation
	 */
	double runPitchDetectionAlgorithm();

private:
    // ** PITCH DETECTION PARAMETERS ** //
	double				_sampleFrequency;						//!< PortAudio stream

    // ** FFTW STRUCTURES ** //
	fftw_plan			_fftw_plan_FFT;							//!< Plan to compute the FFT of a given signal
	fftw_plan			_fftw_plan_IFFT;						//!< Plan to compute the IFFT of a given signal (with additional zero-padding
	double*				_fftw_in_time;							//!< External buffer used to store signals in the time domain (first the input signal and then its autocorrelation)
	unsigned int		_fftw_in_time_size;						//!< Size of the external buffer
	fftw_complex*		_fftw_out_freq;							//!< Buffer used to store signals in the frequency domain (first the FFT of the input signal and later the FFT of its autocorrelation
};

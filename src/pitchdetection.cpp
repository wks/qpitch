#include "pitchdetection.h"

#include <QtAssert>
#include <cstring>
#include <algorithm>
#include <cmath>

const int PitchDetectionContext::ZERO_PADDING_FACTOR = 80;

PitchDetectionContext::PitchDetectionContext(uint32_t sampleFrequency, size_t fftFrameSize) {
    _sampleFrequency = sampleFrequency;
    _fftFrameSize  = fftFrameSize;
    size_t outFrameSize = fftFrameSize * ZERO_PADDING_FACTOR;

    // ** INITIALIZE FFT STRUCTURES ** //
    _window                 = (double*)       fftw_malloc(sizeof(double)       * fftFrameSize);
    _fftw_in_time           = (double*)       fftw_malloc(sizeof(double)       * fftFrameSize);
    _fftw_mid_freq          = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * fftFrameSize);
    _fftw_mid_freq2         = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * outFrameSize);
    _fftw_out_time_autocorr = (double*)       fftw_malloc(sizeof(double)       * outFrameSize);

    _fftw_plan_FFT  = fftw_plan_dft_r2c_1d( fftFrameSize, _fftw_in_time, _fftw_mid_freq, FFTW_ESTIMATE );               // FFT
    _fftw_plan_IFFT = fftw_plan_dft_c2r_1d( outFrameSize, _fftw_mid_freq2, _fftw_out_time_autocorr, FFTW_ESTIMATE );    // IFFT zero-padded

    generateHanningWindow(_window, fftFrameSize);
}

PitchDetectionContext::~PitchDetectionContext() {
    // ** DESTROY FFTW STRUCTURES ** //
    fftw_destroy_plan( _fftw_plan_FFT );
    fftw_destroy_plan( _fftw_plan_IFFT );
    fftw_free(_window);
    fftw_free(_fftw_in_time);
    fftw_free(_fftw_mid_freq);
    fftw_free(_fftw_mid_freq2);
    fftw_free(_fftw_out_time_autocorr);
}

size_t PitchDetectionContext::getFFTFrameSize() const {
    return _fftFrameSize;
}

size_t PitchDetectionContext::getOutFrameSize() const {
    return _fftFrameSize * ZERO_PADDING_FACTOR;
}

void PitchDetectionContext::loadSamples(float *samples, size_t inputSize) {
    size_t numCopy = std::min(inputSize, _fftFrameSize);
    for (size_t i = 0; i < numCopy; i++) {
        _fftw_in_time[i] = samples[i] * _window[i];
    }
    if (inputSize < _fftFrameSize) {
        std::fill(&_fftw_in_time[inputSize], &_fftw_in_time[_fftFrameSize], 0);
    }
}

double* PitchDetectionContext::getInputBuffer() {
    return _fftw_in_time;
}

fftw_complex* PitchDetectionContext::getFreq2Buffer() {
    return _fftw_mid_freq2;
}

double* PitchDetectionContext::getAutoCorrBuffer() {
    return _fftw_out_time_autocorr;
}

double PitchDetectionContext::runPitchDetectionAlgorithm( )
{
    // ** ENSURE THAT FFTW STRUCTURES ARE VALID ** //
    Q_ASSERT(_fftw_plan_FFT          != nullptr);
    Q_ASSERT(_fftw_plan_IFFT         != nullptr);
    Q_ASSERT(_fftw_in_time           != nullptr);
    Q_ASSERT(_fftw_mid_freq          != nullptr);
    Q_ASSERT(_fftw_mid_freq2         != nullptr);
    Q_ASSERT(_fftw_out_time_autocorr != nullptr);

    // ** COMPUTE THE AUTOCORRELATION ** //
    // compute the FFT of the input signal
    fftw_execute( _fftw_plan_FFT );

    /*
     * compute the transform of the autocorrelation given in time domain by
     *
     *        k=-N
     * r[t] = sum( x[k] * x[t-k] )
     *         N
     *
     * or in the frequency domain (for a real signal) by
     *
     * R[f] = X[f] * X[f]' = |X[f]|^2 = Re(X[f])^2 + Im(X[f])^2
     *
     * when computing the FFT with fftw_plan_dft_r2c_1d there are only N/2
     * significant samples so we only need to compute the |.|^2 for
     * _fftw_in_time_Size/2 samples
     */

    // compute |.|^2 of the signal
    for( unsigned int k = 0 ; k < (_fftFrameSize / 2 + 1) ; ++k ) {
        _fftw_mid_freq2[k][0] = (_fftw_mid_freq[k][0] * _fftw_mid_freq[k][0]) + (_fftw_mid_freq[k][1] * _fftw_mid_freq[k][1]);
        _fftw_mid_freq2[k][1] = 0.0;
    }

    // pad the FFT with zeros to increase resolution
    memset( &(_fftw_mid_freq2[_fftFrameSize/ 2 + 1][0]), 0, ( (ZERO_PADDING_FACTOR - 1) * _fftFrameSize + _fftFrameSize/ 2 - 1) * sizeof(fftw_complex) );

    // compute the IFFT to obtain the autocorrelation in time domain
    fftw_execute( _fftw_plan_IFFT );

    // find the maximum of the autocorrelation (rejecting the first peak)
    /*
     * the main problem with autocorrelation techniques is that a peak may also
     * occur at sub-harmonics or harmonics, but right now I can't come up with
     * anything better =(
     */

    // search for a minimum in the autocorrelation to reject the peak centered around 0
    unsigned int l;
    for ( l = 0 ; (l < ( (ZERO_PADDING_FACTOR / 2) * _fftFrameSize + 1)) && ( (_fftw_out_time_autocorr[l+1] < _fftw_out_time_autocorr[l]) || (_fftw_out_time_autocorr[l+1] > 0.0) ) ; ++l ) {};

    // search for the maximum
    double          maxAutoCorrelation          = 0.0;
    unsigned int    maxAutoCorrelation_index    = 0;
    for (  ; l < ( (ZERO_PADDING_FACTOR / 2) * _fftFrameSize + 1) ; ++l ) {
        if ( _fftw_out_time_autocorr[l] > maxAutoCorrelation ) {
            maxAutoCorrelation          = _fftw_out_time_autocorr[l];
            maxAutoCorrelation_index    = l;
        }
    }

    // compute the frequency of the maximum considering the padding factor
    return ( (ZERO_PADDING_FACTOR / 2) * (2.0 * _sampleFrequency) / (double) maxAutoCorrelation_index );
}

void PitchDetectionContext::generateHanningWindow(double *buffer, size_t size) {
    if (size <= 1) {
        // Pathological case.  Just make it a rect window.
        for (size_t i = 0; i < size; i++) {
            buffer[i] = 1.0;
        }
        return;
    }

    for (size_t i = 0; i < size; i++) {
        double x = 2.0 * M_PI * i / (size - 1);
        double y = 0.5 - 0.5 * cos(x);
        buffer[i] = y;
    }
}

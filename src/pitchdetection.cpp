#include "pitchdetection.h"

#include <QtAssert>
#include <cstring>

const int PitchDetectionContext::ZERO_PADDING_FACTOR = 80;

PitchDetectionContext::PitchDetectionContext(uint32_t sampleFrequency, size_t fftFrameSize) {
    _sampleFrequency = sampleFrequency;
    _fftw_in_time_size  = fftFrameSize;         // size of the external buffer (default 2048)

    // ** INITIALIZE FFT STRUCTURES ** //
    _fftw_in_time   = (double*) fftw_malloc( ZERO_PADDING_FACTOR * sizeof(double) * _fftw_in_time_size );
    _fftw_out_freq  = (fftw_complex*) fftw_malloc( ZERO_PADDING_FACTOR * sizeof(fftw_complex) * _fftw_in_time_size );
    _fftw_plan_FFT  = fftw_plan_dft_r2c_1d( _fftw_in_time_size, _fftw_in_time, _fftw_out_freq, FFTW_ESTIMATE );                         // FFT
    _fftw_plan_IFFT = fftw_plan_dft_c2r_1d( ZERO_PADDING_FACTOR * _fftw_in_time_size, _fftw_out_freq, _fftw_in_time, FFTW_ESTIMATE );   // IFFT zero-padded
}

PitchDetectionContext::~PitchDetectionContext() {
    // ** DESTROY FFTW STRUCTURES ** //
    fftw_destroy_plan( _fftw_plan_FFT );
    fftw_destroy_plan( _fftw_plan_IFFT );
    fftw_free( _fftw_in_time );
    fftw_free( _fftw_out_freq );
}

double* PitchDetectionContext::getInputBuffer() {
    return _fftw_in_time;
}

double PitchDetectionContext::runPitchDetectionAlgorithm( )
{
    // ** ENSURE THAT FFTW STRUCTURES ARE VALID ** //
    Q_ASSERT( _fftw_plan_FFT    != NULL );
    Q_ASSERT( _fftw_plan_IFFT   != NULL );
    Q_ASSERT( _fftw_in_time     != NULL );
    Q_ASSERT( _fftw_out_freq    != NULL );

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
    for( unsigned int k = 0 ; k < (_fftw_in_time_size / 2 + 1) ; ++k ) {
        _fftw_out_freq[k][0] = (_fftw_out_freq[k][0] * _fftw_out_freq[k][0]) + (_fftw_out_freq[k][1] * _fftw_out_freq[k][1]);
        _fftw_out_freq[k][1] = 0.0;
    }

    // pad the FFT with zeros to increase resolution
    memset( &(_fftw_out_freq[_fftw_in_time_size/ 2 + 1][0]), 0, ( (ZERO_PADDING_FACTOR - 1) * _fftw_in_time_size + _fftw_in_time_size/ 2 - 1) * sizeof(fftw_complex) );

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
    for ( l = 0 ; (l < ( (ZERO_PADDING_FACTOR / 2) * _fftw_in_time_size + 1)) && ( (_fftw_in_time[l+1] < _fftw_in_time[l]) || (_fftw_in_time[l+1] > 0.0) ) ; ++l ) {};

    // search for the maximum
    double          maxAutoCorrelation          = 0.0;
    unsigned int    maxAutoCorrelation_index    = 0;
    for (  ; l < ( (ZERO_PADDING_FACTOR / 2) * _fftw_in_time_size + 1) ; ++l ) {
        if ( _fftw_in_time[l] > maxAutoCorrelation ) {
            maxAutoCorrelation          = _fftw_in_time[l];
            maxAutoCorrelation_index    = l;
        }
    }

    // compute the frequency of the maximum considering the padding factor
    return ( (ZERO_PADDING_FACTOR / 2) * (2.0 * _sampleFrequency) / (double) maxAutoCorrelation_index );
}


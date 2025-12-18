#include "pitchdetection.h"

#include <QtAssert>
#include <cstring>
#include <algorithm>
#include <cmath>

const int PitchDetectionContext::ZERO_PADDING_FACTOR = 80;

PitchDetectionContext::PitchDetectionContext(uint32_t sampleFrequency, size_t fftFrameSize)
{
    _sampleFrequency = sampleFrequency;
    _fftFrameSize = fftFrameSize;
    size_t outFrameSize = fftFrameSize * ZERO_PADDING_FACTOR;

    // ** INITIALIZE FFT STRUCTURES ** //
    _window = (double *)fftw_malloc(sizeof(double) * fftFrameSize);
    _fftwInTime = (double *)fftw_malloc(sizeof(double) * fftFrameSize);
    _fftwMidFreq = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * fftFrameSize);
    _fftwMidFreq2 = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * outFrameSize);
    _fftwOutTimeAutocorr = (double *)fftw_malloc(sizeof(double) * outFrameSize);

    _fftwPlanFFT =
            fftw_plan_dft_r2c_1d(fftFrameSize, _fftwInTime, _fftwMidFreq, FFTW_ESTIMATE); // FFT
    _fftwPlanIFFT = fftw_plan_dft_c2r_1d(outFrameSize, _fftwMidFreq2, _fftwOutTimeAutocorr,
                                         FFTW_ESTIMATE); // IFFT zero-padded

    generateHanningWindow(_window, fftFrameSize);
}

PitchDetectionContext::~PitchDetectionContext()
{
    // ** DESTROY FFTW STRUCTURES ** //
    fftw_destroy_plan(_fftwPlanFFT);
    fftw_destroy_plan(_fftwPlanIFFT);
    fftw_free(_window);
    fftw_free(_fftwInTime);
    fftw_free(_fftwMidFreq);
    fftw_free(_fftwMidFreq2);
    fftw_free(_fftwOutTimeAutocorr);
}

size_t PitchDetectionContext::getFFTFrameSize() const
{
    return _fftFrameSize;
}

size_t PitchDetectionContext::getOutFrameSize() const
{
    return _fftFrameSize * ZERO_PADDING_FACTOR;
}

void PitchDetectionContext::loadSamples(float *samples, size_t inputSize)
{
    size_t numCopy = std::min(inputSize, _fftFrameSize);
    for (size_t i = 0; i < numCopy; i++) {
        _fftwInTime[i] = samples[i] * _window[i];
    }
    if (inputSize < _fftFrameSize) {
        std::fill(&_fftwInTime[inputSize], &_fftwInTime[_fftFrameSize], 0);
    }
}

double *PitchDetectionContext::getInputBuffer()
{
    return _fftwInTime;
}

fftw_complex *PitchDetectionContext::getFreq2Buffer()
{
    return _fftwMidFreq2;
}

double *PitchDetectionContext::getAutoCorrBuffer()
{
    return _fftwOutTimeAutocorr;
}

double PitchDetectionContext::runPitchDetectionAlgorithm()
{
    // ** ENSURE THAT FFTW STRUCTURES ARE VALID ** //
    Q_ASSERT(_fftwPlanFFT != nullptr);
    Q_ASSERT(_fftwPlanIFFT != nullptr);
    Q_ASSERT(_fftwInTime != nullptr);
    Q_ASSERT(_fftwMidFreq != nullptr);
    Q_ASSERT(_fftwMidFreq2 != nullptr);
    Q_ASSERT(_fftwOutTimeAutocorr != nullptr);

    // ** COMPUTE THE AUTOCORRELATION ** //
    // compute the FFT of the input signal
    fftw_execute(_fftwPlanFFT);

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
    for (unsigned int k = 0; k < (_fftFrameSize / 2 + 1); ++k) {
        _fftwMidFreq2[k][0] = (_fftwMidFreq[k][0] * _fftwMidFreq[k][0])
                + (_fftwMidFreq[k][1] * _fftwMidFreq[k][1]);
        _fftwMidFreq2[k][1] = 0.0;
    }

    // pad the FFT with zeros to increase resolution
    memset(&(_fftwMidFreq2[_fftFrameSize / 2 + 1][0]), 0,
           ((ZERO_PADDING_FACTOR - 1) * _fftFrameSize + _fftFrameSize / 2 - 1)
                   * sizeof(fftw_complex));

    // compute the IFFT to obtain the autocorrelation in time domain
    fftw_execute(_fftwPlanIFFT);

    // find the maximum of the autocorrelation (rejecting the first peak)
    /*
     * the main problem with autocorrelation techniques is that a peak may also
     * occur at sub-harmonics or harmonics, but right now I can't come up with
     * anything better =(
     */

    // search for a minimum in the autocorrelation to reject the peak centered around 0
    unsigned int l;
    for (l = 0; (l < ((ZERO_PADDING_FACTOR / 2) * _fftFrameSize + 1))
         && ((_fftwOutTimeAutocorr[l + 1] < _fftwOutTimeAutocorr[l])
             || (_fftwOutTimeAutocorr[l + 1] > 0.0));
         ++l) { };

    // search for the maximum
    double maxAutoCorrelation = 0.0;
    unsigned int maxAutoCorrelation_index = 0;
    for (; l < ((ZERO_PADDING_FACTOR / 2) * _fftFrameSize + 1); ++l) {
        if (_fftwOutTimeAutocorr[l] > maxAutoCorrelation) {
            maxAutoCorrelation = _fftwOutTimeAutocorr[l];
            maxAutoCorrelation_index = l;
        }
    }

    // compute the frequency of the maximum considering the padding factor
    return ((ZERO_PADDING_FACTOR / 2) * (2.0 * _sampleFrequency)
            / (double)maxAutoCorrelation_index);
}

void PitchDetectionContext::generateHanningWindow(double *buffer, size_t size)
{
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

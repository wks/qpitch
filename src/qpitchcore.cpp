/*
 * QPitch 1.0.1 - Simple chromatic tuner
 * Copyright (C) 1999-2009 William Spinelli <wylliam@tiscali.it>
 *                         Florian Berger <harpin_floh@yahoo.de>
 *                         Reinier Lamers <tux_rocker@planet.nl>
 *                         Pierre Dumuid
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "qpitchcore.h"

#include <QMessageBox>
#include <QMutex>
#include <QtDebug>
#include <QMutexLocker>
#include <chrono>

#ifdef _REFERENCE_SQUAREWAVE_INPUT
    #include <cmath>
#endif

class QPitchCorePrivate {

};


QPitchCore::QPitchCore( QObject* parent, const unsigned int plotPlot_size, QPitchCoreOptions options) :
    QThread( parent ),
    _bufferUpdated(false),
    _stopRequested(false),
    _options(options),
    _stream(nullptr),
    _buffer(0),
    _visualizationData(plotPlot_size),
    _callbackProfilingEnabled(false),
    _callbackProfilingStarted(false),
    _lastCallbackTime(0.0),
    _lastAdcTime(0.0),
    _callbackProfiler("QPitchCore callback", true)
{
    {
        const char *profEnv = getenv("QPITCH_CORE_CALLBACK_PROFILING");
        if (profEnv != nullptr) {
            if (strcmp(profEnv, "1") == 0) {
                qDebug("[QPitchCore callback] Profiling enabled!");
                _callbackProfilingEnabled = true;
            }
        }
    }

    // ** INITIALIZE PRIVATE VARIABLES ** //
    _private = std::make_unique<QPitchCorePrivate>();

    // ** INITIALIZE PORTAUDIO ** //
    PaError err = Pa_Initialize( );
    if ( err != paNoError ) {
        throw QPaSoundInputException( Pa_GetErrorText( err ) );
    }

    reconfigure();

    Q_ASSERT(!_stopRequested);
}


QPitchCore::~QPitchCore( )
{
    // ** ENSURE THAT THE STREAM IS STOPPED AND THE THREAD NOT RUNNING ** //
    Q_ASSERT( _stream == nullptr );
    Q_ASSERT( _stopRequested == true );
    Q_ASSERT( ! this->isRunning( ) );

    // ** TERMINATE PORTAUDIO ** //
    Pa_Terminate( );

    // ** RELEASE RESOURCES ** //
}

void QPitchCore::setOptions(QPitchCoreOptions options) {
    QMutexLocker locker(&_mutex);
    _pendingOptions = options;
    _cond.wakeOne();
}

void QPitchCore::requestStop() {
    QMutexLocker locker(&_mutex);
    _stopRequested = true;
    _cond.wakeOne();
}

void QPitchCore::setCallbackProfilingEnabled(bool enabled) {
    _callbackProfilingEnabled = enabled;
}

void QPitchCore::startStream()
{
    // This method is only callable by the QPitchCore thread itself.
    Q_ASSERT( QThread::currentThread() == this );

    // The stream should not have been started.
    Q_ASSERT( _stream == NULL );

#ifdef _REFERENCE_SQUAREWAVE_INPUT
    // create the artificial square wave with some harmonics :: 110.0 Hz
    for ( unsigned int k = 0 ; k < 4410 ; ++k ) {
        _referenceSineWave[k] = ( (sin( 2 * M_PI * 110.0 * (double) k / sampleFrequency ) >= 0.0) ? 1000 : -1000 );
    }
    _referenceSineWave_index = 0;
#endif

    // Parameters of the input audio stream
    PaStreamParameters  inputParameters;

    // We dump the host API and device list for debug purposes.
    qDebug("Enumerating host APIs...");
    for (int i = 0, end = Pa_GetHostApiCount(); i != end; ++i) {
        const PaHostApiInfo *hostAPIInfo = Pa_GetHostApiInfo(i);
        qDebug("Host API %d: %s", i, hostAPIInfo->name);
        for (int j = 0, end = hostAPIInfo->deviceCount; j != end; ++j) {
            PaDeviceIndex deviceIndex = Pa_HostApiDeviceIndexToDeviceIndex(i, j);
            const PaDeviceInfo *info = Pa_GetDeviceInfo(deviceIndex);
            if (info->maxInputChannels > 1) {
                qDebug("  %d: [%d] %s (%d channels, %lf Hz, %lf ms to %lf ms)",
                    j, deviceIndex, info->name,
                    info->maxInputChannels, info->defaultSampleRate,
                    info->defaultLowInputLatency * 1000, info->defaultHighInputLatency * 1000);
            } else {
                qDebug("  %d: [%d] %s (no input)", j, deviceIndex, info->name);
            }
        }
    }

    // Prefer the default device.  On Linux, the default host API is ALSA; and on modern Linux
    // distributions, the default output device is usually "pipewire" which bridges with the
    // PipeWire sound server.
    //
    // TODO: Allow the user to specify a device at runtime via the GUI.
    inputParameters.device = -1;

    // We list the devices for debug purpose.
    qDebug("Enumerating PortAudio devices...");
    for (int i = 0, end = Pa_GetDeviceCount(); i != end; ++i) {
        PaDeviceInfo const* info = Pa_GetDeviceInfo(i);
        if (!info) {
            qDebug("  [%d] no info", i);
            continue;
        }
        qDebug("  [%d] name: %s", i, info->name);
    }

    qDebug("Default device: %d", Pa_GetDefaultInputDevice());
    qDebug("Selected device: %d", inputParameters.device);

    // ** CONFIGURE THE INPUT AUDIO STREAM ** //
    if (inputParameters.device == -1) {
        inputParameters.device                  =   Pa_GetDefaultInputDevice( );                // default input device
    }
    inputParameters.channelCount                =   1;                                          // mono input
    inputParameters.sampleFormat                =   PA_SAMPLE_FORMAT;                           // the one we specified
    inputParameters.suggestedLatency            =   1.0 / 60.0;                                 // Try to get 60 fps.
    inputParameters.hostApiSpecificStreamInfo   =   NULL;

    // ** OPEN AN AUDIO INPUT STREAM ** //

    // We don't specify the buffer size.  Pa_OpenStream promises that by doing so, "the stream
    // callback will receive an optimal (and possibly varying) number of frames based on host
    // requirements and the requested latency settings", and "the use of non-zero framesPerBuffer
    // for a callback stream may introduce an additional layer of buffering which could introduce
    // additional latency". Meanwhile, since we are using a cyclic buffer to hold accumulated
    // samples, we are quite flexible about the buffer size, and we can even handle variable-sized
    // buffers with ease. Therefore, not specifying the buffer size will allow us to run at the
    // maximum possible FPS.  On a machine with Linux and PipeWire, we receive about 395 callbacks
    // per second, with the number of frames per callback ranging from 7 to 183.
    //
    // TODO: Allow the user to set (throttle) the frequency at which the QPitchCore processes the
    // buffer and therefore the GUI refresh rate to the user's desired setting, such as 60 FPS.
    unsigned long framesPerBuffer = paFramesPerBufferUnspecified;
    PaError err = Pa_OpenStream(
        &_stream,
        &inputParameters,
        NULL,                                   // no output
        _options.sampleFrequency,               // sample rate (default 44100 Hz)
        framesPerBuffer,                        // frames per buffer (not specified)
        paClipOff,                              // disable clipping
        paCallback,                             // callback
        this                                    // pointer to user data
    );

    if ( err != paNoError ) {
        throw QPaSoundInputException( Pa_GetErrorText( err ) );
    }

    // ** START PORTAUDIO STREAM ** //
    err = Pa_StartStream( _stream );
    if( err != paNoError ) {
        throw QPaSoundInputException( Pa_GetErrorText( err ) );
    }

    // ** ENSURE THAT THE STREAM IS STARTED AND THE THREAD IS RUNNING ** //
    Q_ASSERT( _stream != nullptr );

    const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo( inputParameters.device );
    QString device = QString(deviceInfo->name);
    QString hostAPI = QString(Pa_GetHostApiInfo( Pa_GetDeviceInfo( inputParameters.device )->hostApi )->name);
    emit portAudioStreamStarted(device, hostAPI);

    qDebug( ) << "QPitchCore::startStream";
    qDebug( ) << " - sampleFrequency  = " << _options.sampleFrequency;
    qDebug( ) << " - suggestedLatency = " << inputParameters.suggestedLatency;
    qDebug( ) << " - fftFrameSize     = " << _options.fftFrameSize << "\n";
}


void QPitchCore::stopStream( )
{
    // This method is only callable by the QPitchCore thread itself.
    Q_ASSERT( QThread::currentThread() == this );

    // ** ENSURE THAT THE STREAM IS STARTED ** //
    Q_ASSERT( _stream           != NULL );

    // ** STOP PORTAUDIO STREAM ** //
    PaError err = Pa_StopStream( _stream );
    if( err != paNoError ) {
        throw QPaSoundInputException( Pa_GetErrorText( err ) );
    }

    // ** CLOSE PORTAUDIO STREAM ** //
    err = Pa_CloseStream( _stream );
    if( err != paNoError ) {
        throw QPaSoundInputException( Pa_GetErrorText( err ) );
    }

    // ** RELEASE PITCH DETECTION INSTANCE ** //
    _pitchDetection.reset();

    // ** RELEASE RESOURCES ** //
    _stream         = NULL;
}

int QPitchCore::paCallback( const void* input, void* /*output*/, unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData )
{
    Q_ASSERT( input     != NULL );
    Q_ASSERT( userData  != NULL );

    return( static_cast<QPitchCore*>( userData )->paStoreInputBufferCallback( (const SampleType*)input, frameCount,
        timeInfo, statusFlags ) );
}

int QPitchCore::paStoreInputBufferCallback( const SampleType* input, unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags)
{
    Q_ASSERT(QThread::currentThread() != this);

    bool profilingEnabled = _callbackProfilingEnabled;

    std::chrono::time_point<std::chrono::steady_clock> callbackEnter;

    if (profilingEnabled) {
        _callbackProfiler.tick();

        if (!_callbackProfilingStarted) {
            _callbackProfilingStarted = true;
        } else {
            double callbackDiff = timeInfo->currentTime - _lastCallbackTime;
            double adcDiff = timeInfo->inputBufferAdcTime - _lastAdcTime;
            qDebug("[QPitchCore callback] Time profiling: callback: %lf (+%lf), ADC: %lf (+%lf), frames: %ld",
                timeInfo->currentTime, callbackDiff, timeInfo->inputBufferAdcTime, adcDiff, frameCount);
        }

        _lastCallbackTime = timeInfo->currentTime;
        _lastAdcTime = timeInfo->inputBufferAdcTime;

        callbackEnter = std::chrono::steady_clock::now();
    }

    {
        QMutexLocker bufferLocker(&_bufferMutex);

        // ** COPY BUFFER ** //
#ifdef _REFERENCE_SQUAREWAVE_INPUT
        // ** USE THE REFERENCE SINE WAVE INPUT SIGNAL ** //
        for ( unsigned int k = 0 ; k < frameCount ; ++k ) {
            _buffer[k] = _referenceSineWave[_referenceSineWave_index++];
            if ( _referenceSineWave_index >= 4410 ) {
                _referenceSineWave_index = 0;
            }
        }
#else
        // ** READ THE REAL AUDIO SIGNAL ** //
        _buffer.append((const unsigned char*)input, frameCount * sizeof( SampleType ) );
#endif
    }

    // ** NOTIFY THE QPITCHCORE THREAD TO PROCESS THE BUFFER ** //
    bool bufferWasUpdated = false;
    {
        QMutexLocker locker(&_mutex);

        // Note that the callback can update the buffer faster than the QPitchCore thread can
        // handle. This is normal.  Samples will be accumulated into the cyclic buffer, and the
        // QPitchCore thread always processes the accumulated samples.  But for performance
        // analysis, we record the old value of _bufferUpdated to see if the QPitchCore can keep up
        // with the callback.
        bufferWasUpdated = _bufferUpdated;

        _bufferUpdated = true;
        _cond.wakeOne( );
    }

    if (profilingEnabled) {
        std::chrono::time_point<std::chrono::steady_clock> callbackExit = std::chrono::steady_clock::now();
        double callbackDuration = std::chrono::duration<double>(callbackExit - callbackEnter).count();
        qDebug("[QPitchCore callback] Callback duration: %lf", callbackDuration);

        if ( _bufferUpdated ) {
            // buffer is not processed since the last callback
            qDebug("[QPitchCore callback] The QPitchCore thread failed to keep up with the callback!");
        }
    }

    return paContinue;
}


void QPitchCore::run( )
{
    startStream();

    // ** ENSURE THAT FFTW STRUCTURES ARE VALID ** //
    Q_ASSERT( _pitchDetection );

    {
        QMutexLocker locker(&_mutex);
        while(true) {
            // Wait until either the buffer is updated or _running is set to false.
            while (!_stopRequested && !_pendingOptions && !_bufferUpdated) {
                _cond.wait( &_mutex );
            }

            // lock the buffer
            if ( _stopRequested ) {
                qDebug("Stop requested! Stop!");
                break;
            }

            if (_pendingOptions) {
                onOptionsChanged(locker);
            }

            if (_bufferUpdated) {
                processBuffer(locker);
            }
        }
    }

    stopStream();

    // TODO: Signal stopped event.
}

void QPitchCore::onOptionsChanged(QMutexLocker<QMutex> &locker) {
    // Prevent deadlock with PortAudio's callback thread.
    locker.unlock();

    qDebug("Options changed.");
    stopStream();

    locker.relock();
    // Note: The GUI thread may have modified _pendingOptions while we unlocked.
    // It doesn't matter, as long as we take the latest version.
    Q_ASSERT(_pendingOptions);
    _options = std::move(_pendingOptions.value());
    _pendingOptions.reset();
    locker.unlock();

    reconfigure();

    startStream();

    locker.relock();
}

void QPitchCore::reconfigure() {
    // The stream must be stopped.
    Q_ASSERT(_stream == nullptr);

    // ** INITIALIZE BUFFERS ** //
    _buffer = CyclicBuffer(_options.fftFrameSize * sizeof(SampleType));
    _tmp_sample_buffer.clear();
    _tmp_sample_buffer.resize(_options.fftFrameSize);

    // ** CREATE THE PITCH DETECTION INSTANCE ** //
    _pitchDetection = std::make_unique<PitchDetectionContext>(_options.sampleFrequency, _options.fftFrameSize);
}

void QPitchCore::processBuffer(QMutexLocker<QMutex> &locker) {
    Q_ASSERT(_bufferUpdated);
    Q_ASSERT(_pitchDetection);

    // No need to keep the lock when we copy the buffer contents.
    locker.unlock();

    size_t frames_copied;
    {
        QMutexLocker bufferLocker(&_bufferMutex);
        // Dump the samples out of the cyclic buffer.
        size_t bytes_copied = _buffer.copyLastBytes((unsigned char*)_tmp_sample_buffer.data(), _options.fftFrameSize * sizeof(SampleType));
        frames_copied = bytes_copied / sizeof(SampleType);
    }

    {
        locker.relock();
        // This is for notifying the callback thread.
        _bufferUpdated = false;
        locker.unlock();
    }

    // Transfer the samples to _pitchDetection, converting sample format (float -> double) at the same time.
    double *fftw_in_time = _pitchDetection->getInputBuffer();
    memset(fftw_in_time, 0, _options.fftFrameSize * sizeof(double));
    for (size_t k = 0; k < frames_copied; k++) {
        fftw_in_time[k] = _tmp_sample_buffer[k];
    }

    // downsample factor used to extract a buffer with a time range of 50 milliseconds
    unsigned int fftw_in_downsampleFactor;
    if ( _options.sampleFrequency == 44100.0 ) {
        fftw_in_downsampleFactor = 4;
    } else if ( _options.sampleFrequency == 22050.0 ) {
        fftw_in_downsampleFactor = 2;
    }

    {
        QMutexLocker visDataLocker(&_visualizationData.mutex);

        // Copy some samples to _visualizationData.
        {
            double lastSample = fftw_in_time[0];
            bool risingEdgeDetected = false;
            size_t plotSampleCursor = 0;
            for ( unsigned int k = 1; k < frames_copied; ++k ) {
                // Find the first rising edge that crosses the zero point.
                double currentSample = fftw_in_time[k];
                if (lastSample < 0.0 && currentSample >= 0.0) {
                    risingEdgeDetected = true;
                }
                if (risingEdgeDetected) {
                    // TODO: See why the qosziview assumes the -32768 to 32767 range.
                    _visualizationData.plotSample[plotSampleCursor] = currentSample;
                    plotSampleCursor++;
                    if (plotSampleCursor >= _visualizationData.plotData_size) {
                        break;
                    }
                }
            }
        }

        _visualizationData.timeRangeSample = _options.fftFrameSize / _options.sampleFrequency;

        // compute the autocorrelation and find the best matching frequency
        _visualizationData.estimatedFrequency = _pitchDetection->runPitchDetectionAlgorithm( );

        // extract autocorrelation samples for the oscilloscope view in the range [40, 1000] Hz --> [0, 25] msec
        unsigned int fftw_out_downsampleFactor;
        if ( _options.sampleFrequency == 44100.0 ) {
            fftw_out_downsampleFactor = 2 * PitchDetectionContext::ZERO_PADDING_FACTOR;
        } else if ( _options.sampleFrequency == 22050.0 ) {
            fftw_out_downsampleFactor = 1 * PitchDetectionContext::ZERO_PADDING_FACTOR;
        }

        for ( unsigned int k = 0 ; k < _visualizationData.plotData_size ; ++k ) {
            Q_ASSERT( (k * fftw_out_downsampleFactor) < (PitchDetectionContext::ZERO_PADDING_FACTOR * _options.fftFrameSize) );
            _visualizationData.plotAutoCorr[k] = fftw_in_time[k * fftw_out_downsampleFactor];
        }

        _visualizationData.estimatedNote = _options.tuningParameters.estimateNote(_visualizationData.estimatedFrequency);
    }

    emit visualizationDataUpdated(&_visualizationData);

    locker.relock();
}


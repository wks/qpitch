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

#ifdef _REFERENCE_SQUAREWAVE_INPUT
	#include <cmath>
#endif


// ** INITIALIZATION OF STATIC VARIABLES ** //
const int QPitchCore::SIGNAL_THRESHOLD_ON	= 100;
const int QPitchCore::SIGNAL_THRESHOLD_OFF	= 20;


QPitchCore::QPitchCore( QObject* parent, const unsigned int plotPlot_size, TuningParameters tuningParameters) :
	QThread( parent ),
	_visualizationData(plotPlot_size),
	_tuningParameters(tuningParameters),
	_buffer(0)
{
	// ** INITIALIZE PRIVATE VARIABLES ** //
	_stream			= NULL;

	// ** INITIALIZE PORTAUDIO ** //
	PaError err = Pa_Initialize( );
	if ( err != paNoError ) {
		throw QPaSoundInputException( Pa_GetErrorText( err ) );
	}
}


QPitchCore::~QPitchCore( )
{
	// ** ENSURE THAT THE STREAM IS STOPPED AND THE THREAD NOT RUNNING ** //
	Q_ASSERT( _stream		== NULL );
	Q_ASSERT( _running		== false );
	Q_ASSERT( ! this->isRunning( ) );

	// ** TERMINATE PORTAUDIO ** //
	Pa_Terminate( );

	// ** RELEASE RESOURCES ** //
}


void QPitchCore::startStream( const unsigned int sampleFrequency, const unsigned int fftFrameSize )
{
	// ** ENSURE THAT THE STREAM IS STOPPED AND THE THREAD IS NOT RUNNING ** //
	Q_ASSERT( _stream == NULL );
	Q_ASSERT( ! this->isRunning( ) );

#ifdef _REFERENCE_SQUAREWAVE_INPUT
	// create the artificial square wave with some harmonics :: 110.0 Hz
	for ( unsigned int k = 0 ; k < 4410 ; ++k ) {
		_referenceSineWave[k] = ( (sin( 2 * M_PI * 110.0 * (double) k / sampleFrequency ) >= 0.0) ? 1000 : -1000 );
	}
	_referenceSineWave_index = 0;
#endif
        _inputParameters.device = -1;
        for (int i = 0, end = Pa_GetDeviceCount(); i != end; ++i) {
            PaDeviceInfo const* info = Pa_GetDeviceInfo(i);
            if (!info) continue;
            if (strcmp(info->name, "pulse") == 0) {
                _inputParameters.device = i;
                break;
            }
        }

	// ** CONFIGURE THE INPUT AUDIO STREAM ** //
	_sampleFrequency							=	sampleFrequency;
        if (_inputParameters.device == -1) {
            _inputParameters.device						=	Pa_GetDefaultInputDevice( );				// default input device
        }
	_inputParameters.channelCount				=	1;											// mono input
	_inputParameters.sampleFormat				=	PA_SAMPLE_FORMAT;							// the one we specified
	_inputParameters.suggestedLatency			=	1.0 / 60.0;									// Try to get 60 fps.
	_inputParameters.hostApiSpecificStreamInfo	=	NULL;

	// ** INITIALIZE BUFFERS ** //
	_buffer = CyclicBuffer(fftFrameSize * sizeof(SampleType));
	_tmp_sample_buffer.clear();
	_tmp_sample_buffer.resize(fftFrameSize);

	_fftFrameSize = fftFrameSize;
	_pitchDetection = std::make_unique<PitchDetectionContext>(sampleFrequency, fftFrameSize);

	// ** OPEN AN AUDIO INPUT STREAM ** //
	_buffer_size = (unsigned int)((double) _inputParameters.suggestedLatency * sampleFrequency);
	PaError err = Pa_OpenStream(
		&_stream,
		&_inputParameters,
		NULL,									// no output
		_sampleFrequency,						// sample rate (default 44100 Hz)
		(long) _buffer_size,					// frames per buffer
		paClipOff,								// disable clipping
        paCallback,                             // callback
		this									// pointer to user data
	);

	if ( err != paNoError ) {
		throw QPaSoundInputException( Pa_GetErrorText( err ) );
	}

	// ** START PORTAUDIO STREAM ** //
	err = Pa_StartStream( _stream );
	if( err != paNoError ) {
		throw QPaSoundInputException( Pa_GetErrorText( err ) );
	}

	// ** START WORKING THREAD ** //
	_running = true;
	this->start( );

	// ** ENSURE THAT THE STREAM IS STARTED AND THE THREAD IS RUNNING ** //
	Q_ASSERT( _stream != NULL );
	Q_ASSERT( this->isRunning( ) );

	qDebug( ) << "QPitchCore::startStream";
	qDebug( ) << " - sampleFrequency         = " << _sampleFrequency;
	qDebug( ) << " - defaultHighInputLatency = " << _inputParameters.suggestedLatency;
	qDebug( ) << " - framesPerBuffer         = " << _buffer_size;
	qDebug( ) << " - fftFrameSize            = " << fftFrameSize << "\n";
}


void QPitchCore::stopStream( )
{
	// ** ENSURE THAT THE STREAM IS STARTED ** //
	Q_ASSERT( _stream			!= NULL );

	// ** STOP THE THREAD ** //
	{
		QMutexLocker locker(&_mutex);
		_running = false;
	}

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

	// FIXME: Wait!  Can we really do this?  The QPitchCore thread is still using the buffers!
	_pitchDetection.reset();

	// ** RELEASE RESOURCES ** //
	_stream			= NULL;
}

void QPitchCore::getPortAudioInfo( QString& device ) const
{
	// ** ENSURE THAT THE STREAM IS STARTED ** //
	Q_ASSERT( _stream != NULL );

	// ** RETRIEVE STREAM PROPERTIES ** //
	device = QString( "Device: " + QString(Pa_GetDeviceInfo( _inputParameters.device )->name) + " [" +
		QString(Pa_GetHostApiInfo( Pa_GetDeviceInfo( _inputParameters.device )->hostApi )->name)  + "]");
}


int QPitchCore::paCallback( const void* input, void* /*output*/, unsigned long frameCount,
		const PaStreamCallbackTimeInfo* /*timeInfo*/, PaStreamCallbackFlags /*statusFlags*/, void* userData )
{
	Q_ASSERT( input		!= NULL );
	Q_ASSERT( userData	!= NULL );

    return( static_cast<QPitchCore*>( userData )->paStoreInputBufferCallback( (const SampleType*)input, frameCount ) );
}

int QPitchCore::paStoreInputBufferCallback( const SampleType* input, unsigned long frameCount )
{
    QMutexLocker locker(&_mutex);
    if ( !_bufferUpdated ) {
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

		_bufferUpdated = true;

        // Let the QPitchCore thread process the filled buffer.
        _waitCond.wakeOne( );
    } else {
        // buffer cannot be locked
        std::cerr << "QPitch: buffer full, dropping " << frameCount << " samples!\n";
    }

	return paContinue;
}


void QPitchCore::run( )
{
	// ** ENSURE THAT FFTW STRUCTURES ARE VALID ** //
	Q_ASSERT( _pitchDetection );

	// initialize the visualization status
	_visualizationStatus = STOPPED;

	QMutexLocker locker(&_mutex);
	forever {
		// Wait until either the buffer is updated or _running is set to false.
		_waitCond.wait( &_mutex );

		// lock the buffer
		if ( _running == false ) {
			_visualizationStatus = STOPPED;
			return;
		}

		Q_ASSERT(_bufferUpdated);

		// ** PROCESS THE BUFFER AND SLEEP TILL THE NEXT FRAME ** //
		// transfer the internal buffer to the external buffer and
		// drop all the samples that exceed its length
		// check if the whole signal is below a given threshold to
		// stop visualization

		// Dump the samples out of the cyclic buffer.
		size_t bytes_copied = _buffer.copyLastBytes((unsigned char*)_tmp_sample_buffer.data(), _fftFrameSize * sizeof(SampleType));
		size_t frames_copied = bytes_copied / sizeof(SampleType);

		// Transfer the samples to _pitchDetection, converting sample format (float -> double) at the same time.
		double *fftw_in_time = _pitchDetection->getInputBuffer();
		memset(fftw_in_time, 0, _fftFrameSize * sizeof(double));
		for (size_t k = 0; k < frames_copied; k++) {
			fftw_in_time[k] = _tmp_sample_buffer[k];
		}

		// Currently we always assume there is a signal.
		// TODO: Find a way detect the volume of the wave, and tell if there is a silence.
		if ( _visualizationStatus == STOPPED ) {
			_visualizationStatus = START_REQUEST;
		}

		// downsample factor used to extract a buffer with a time range of 50 milliseconds
		unsigned int fftw_in_downsampleFactor;
		if ( _sampleFrequency == 44100.0 ) {
			fftw_in_downsampleFactor = 4;
		} else if ( _sampleFrequency == 22050.0 ) {
			fftw_in_downsampleFactor = 2;
		}

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

		_visualizationData.timeRangeSample = _fftFrameSize / _sampleFrequency;

		// compute the autocorrelation and find the best matching frequency
		_visualizationData.estimatedFrequency = _pitchDetection->runPitchDetectionAlgorithm( );

		// extract autocorrelation samples for the oscilloscope view in the range [40, 1000] Hz --> [0, 25] msec
		unsigned int fftw_out_downsampleFactor;
		if ( _sampleFrequency == 44100.0 ) {
			fftw_out_downsampleFactor = 2 * PitchDetectionContext::ZERO_PADDING_FACTOR;
		} else if ( _sampleFrequency == 22050.0 ) {
			fftw_out_downsampleFactor = 1 * PitchDetectionContext::ZERO_PADDING_FACTOR;
		}

		for ( unsigned int k = 0 ; k < _visualizationData.plotData_size ; ++k ) {
			Q_ASSERT( (k * fftw_out_downsampleFactor) < (PitchDetectionContext::ZERO_PADDING_FACTOR * _fftFrameSize) );
			_visualizationData.plotAutoCorr[k] = fftw_in_time[k * fftw_out_downsampleFactor];
		}

		_visualizationData.estimatedNote = _tuningParameters.estimateNote(_visualizationData.estimatedFrequency);

		emit visualizationDataUpdated(&_visualizationData);

		// manage the visualization status
		if ( _visualizationStatus == STOP_REQUEST ) {
			emit updateSignalPresence( false );
			_visualizationStatus = STOPPED;
		} else if ( _visualizationStatus == START_REQUEST ) {
			emit updateSignalPresence( true );
			_visualizationStatus = RUNNING;
		}

		_bufferUpdated = false;
	}
}


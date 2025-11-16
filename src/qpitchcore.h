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

#ifndef __QPITCHCORE_H_
#define __QPITCHCORE_H_

#include "notes.h"
#include "visualization_data.h"
#include "cyclicbuffer.h"
#include "pitchdetection.h"

//! Definition used to feed the application with a reference squarewave
//#define _REFERENCE_SQUAREWAVE_INPUT

#include <cstring>
#include <iostream>
#include <stdexcept>

#include <portaudio.h>

#include <QMessageBox>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

//! An exception thrown when a PortAudio error occurs
class QPaSoundInputException : public std::runtime_error {

public: /* methods */
	//! Default constructor.
	/*!
	 * \param[in] msg the error message to display
	 */
	QPaSoundInputException( const std::string& msg ) : std::runtime_error( msg ) {
		return;
	};

	//! Report exception.
	void report( ) {
		QMessageBox::critical( NULL, "QPitch", QString( "PortAudio error: %1." ).arg( this->what( ) ) );
		std::cerr << "PortAudio error: " << this->what( ) << "\n";
	};
};

//! Options for the QPitchCore thread.
/*!
 * This contains options that are settable by the UI.  The acutal QPitchCore thread may keep a
 * private copy of this in order to work safely concurrently.
 *
 * Some fields may mirror that of QPitchSettings, while others can be run-time options such as
 * the selected device (to be added).
 */
struct QPitchCoreOptions {
	uint32_t sampleFrequency;
	size_t fftFrameSize;
	TuningParameters tuningParameters;
};

// Private implementation.  Not visible from outside.
class QPitchCorePrivate;

//! Working thread for the QPitch application.
/*!
 * This class implements the main working thread for the QPitch
 * application.
 * The audio stream is acquired through the PortAudio library
 * (cross-platform) using a callback function. The size of the internal
 * buffer is set to the size suggested for robust non-interactive
 * application, since the latency is not an issue in this application.
 * In the current version the default audio input stream is used,
 * thus the selection of the audio input is performed using the control
 * panel of the operating system.
 * The pitch detection algorithm is based on the identification of the
 * first peak in the autocorrelation of the signal, which is computed
 * as the inverse FFT of the power spectral density of the signal
 * (the squared module of the signal FFT). The FFT is computed using
 * the FFTW3 library and prior to the inverse transform the signal is
 * zero-padded to increase the resolution of the autocorrelation in
 * order to have a better frequency identification.
 */
class QPitchCore : public QThread {
	Q_OBJECT


#ifdef _REFERENCE_SQUAREWAVE_INPUT
public: /* members */
	short int			_referenceSineWave[4410];				//!< Artificial sine-wave used for debug
	unsigned int		_referenceSineWave_index;				//!< Index incremented after each step to simulate time
#endif

private: /* sample format related */
	using SampleType = float;
	const PaSampleFormat PA_SAMPLE_FORMAT = paFloat32;


public: /* methods */
	//! Default constructor.
	/*!
	 * \param[in] plotPlot_size the number of sample in the buffer used for visualization
	 * \param[in] parent a QObject* with the handle of the parent
	 */
	QPitchCore(QObject* parent, const unsigned int plotPlot_size, QPitchCoreOptions options);

	//! Default destructor.
	~QPitchCore( );

	//! Start an input audio stream with the given properties.
	void startStream();

	//! Stop the input audio stream.
	void stopStream( );

	//! Retrieve the device parameters.
	/*!
	 * \param[out] device the name of the device used by PortAudio plus the name of the API used by PortAudio
	 */
	void getPortAudioInfo( QString& device ) const;

    /*! \brief Dummy callback function to call the real non-static callback that does the work.
     *  \param[in] input Pointer to the interleaved input samples.
     *  \param[out] output Pointer to the interleaved output samples.
     *  \param[in] frameCount Number of sample frames to be processed.
     *  \param[in] timeInfo Time when the buffer is processed.
     *  \param[in] statusFlags Whether underflow or overflow occurred.
     *  \param[in] userData Pointer to user data.
     */
    static int paCallback( const void* input, void* output, unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData );

    /*! \brief Play the selected WAV file through the selected PortAudio device.
     *  \param[in] input Pointer to the interleaved input samples.
     *  \param[in] frameCount Number of sample frames to be processed.
     */
    int paStoreInputBufferCallback( const SampleType* output, unsigned long frameCount );

	void setTuningParameters(TuningParameters tuningParameters);

signals:
	//! Emitted when any part of the visualization data is updated.
	/*!
	 * \param[in] visData a reference to the VisualizationData struct
	 */
	void visualizationDataUpdated(VisualizationData *visData);	//! Signal the level of the input signal.

	/*!
	 * \param[in] signalPresent flag with the current signal presence
	 */
	void updateSignalPresence( bool signalPresent );

protected:
	//! Main loop of the thread.
	virtual void run( );


private: /* enumerations */
	//! Status of the visualization used to handle silence in the input stream.
	enum VisualizationStatus {
		STOPPED,
		STOP_REQUEST,
		START_REQUEST,
		RUNNING
		};

private: /* static constants */
	static const int 	SIGNAL_THRESHOLD_ON;					//!< Value of the threshold above which the processing is activated
	static const int 	SIGNAL_THRESHOLD_OFF;					//!< Value of the threshold below which the input audio signal is deactivated


private: /* members */
	// ** SETTABLE OPTIONS ** //
	QPitchCoreOptions	_options;	// Guarded by _mutex.

	// ** PORTAUDIO STREAM ** //
	PaStreamParameters	_inputParameters;						//!< Parameters of the input audio stream
	PaStream*			_stream;								//!< Handle to the PortAudio stream
	CyclicBuffer        _buffer;								//!< Internal buffer to store the input samples read in the callback
	unsigned int		_buffer_size;							//!< Size of the internal buffer
	std::vector<SampleType>	_tmp_sample_buffer;					//!< A temporary buffer for dumping samples from the cyclic buffer

	// ** FFT ** //
	std::unique_ptr<PitchDetectionContext> _pitchDetection;

	// ** THREAD HANDLING ** //
	bool				_running;								//!< True when the thread is running
	QMutex				_mutex;									//!< Mutex used by the wait condition
	bool 				_bufferUpdated;						    //!< Set to true when the input buffer is filled by the audio backend.
	QWaitCondition		_waitCond;								//!< Wait condition used to put the thread to sleep while waiting for audio samples

	// ** TEMPORARY BUFFERS USED FOR VISUALIZATION ** //
	VisualizationData   _visualizationData;						//!< Visualization data shared with the UI thread
	VisualizationStatus	_visualizationStatus;					//!< Visualization status used to handle silence

private: /* methods */
};
#endif


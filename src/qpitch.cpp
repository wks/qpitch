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

#include "qpitch.h"

#include "qaboutdlg.h"
#include "qsettingsdlg.h"
#include "qpitchcore.h"
#include "fpsprofiler.h"

#include <QSettings>
#include <QTimer>

// ** CONSTANTS ** //
const int QPitch::PLOT_BUFFER_SIZE = 551;		// 44100 * 0.05 / 4 = 551.25
													// size computed to have a time range of 50 ms with an integer downsample ratio
													// sample rate = 44100 Hz --> downsample ratio = 4
													// sample rate = 22050 Hz --> downsample ratio = 2


QPitch::QPitch( QMainWindow* parent ) : QMainWindow( parent )
{
	// ** SETUP THE MAIN WINDOW ** //
	_gt.setupUi( this );

	// ** RETRIEVE APPLICATION SETTINGS ** //
	QSettings settings( "QPitch", "QPitch" );

	// restrict sample frequency to 44100 - 22050 Hz
	unsigned int sampleFrequency = settings.value( "audio/samplefrequency", 44100 ).toUInt( );
	if ( (sampleFrequency != 44100) && (sampleFrequency != 22050) ) {
		// invalid value, set to default (44100 Hz)
		sampleFrequency = 44100;
	}

	// restrict frame buffer size to 8192 - 4096 samples
	unsigned int fftFrameSize = settings.value( "audio/buffersize", 4096 ).toUInt( );
	if ( (fftFrameSize != 8192) && (fftFrameSize != 4096) ) {
		// invalid value, set to default (4096 samples)
		fftFrameSize = 4096;
	}

	// restrict the fundamental frequency to the range [400, 480] Hz
	double fundamentalFrequency = settings.value( "audio/fundamentalfrequency", 440.0 ).toDouble( );
	if ( (fundamentalFrequency > 480.0) || (fundamentalFrequency <= 400.0) ) {
		// invalid value, set to default (440.0 Hz)
		fundamentalFrequency = 440.0;
	}

	// restrict the fundamental TuningNotation to the range 0 (US) - 1 (French) - 2 (German)
	unsigned int tuningNotation = settings.value( "audio/tuningnotation", 0 ).toUInt( );
	if ( !(tuningNotation <= (int)TuningNotation::GERMAN) ) {
		// invalid value, set to default (US)
		tuningNotation = 0;
	}

	// ** REJECT MOUSE EVENT FOR QLINEEDIT ** //
	_gt.lineEdit_note->installEventFilter( this );
	_gt.lineEdit_frequency->installEventFilter( this );

	// ** INITIALIZE TUNING PARAMETERS ** //
	_tuningParameters = std::make_shared<TuningParameters>(fundamentalFrequency, (TuningNotation)tuningNotation);

	// ** INTIALIZE PORTAUDIO STREAM ** //
	try {
		_hQPitchCore = new QPitchCore( this, PLOT_BUFFER_SIZE, TuningParameters(*_tuningParameters) );
	} catch ( QPaSoundInputException& e ) {
		e.report( );
	}

	// ** INITIALIZE CUSTOM WIDGETS ** //
	_gt.widget_qlogview->setTuningParameters(_tuningParameters);
	_gt.widget_qosziview->setBufferSize( PLOT_BUFFER_SIZE );

	// ** SETUP THE CONNECTIONS ** //
	// File menu
	connect( _gt.action_preferences, &QAction::triggered,
		this, &QPitch::showPreferencesDialog);
	connect( _gt.action_compactView, &QAction::triggered,
		this, &QPitch::setViewCompactMode);

	// Help menu
	connect( _gt.action_about, &QAction::triggered,
		this, &QPitch::showAboutDialog);
	connect( _gt.action_aboutQt, &QAction::triggered,
		qApp, &QApplication::aboutQt);

	// Internal connections
	connect( _hQPitchCore, &QPitchCore::visualizationDataUpdated,
		this, &QPitch::onVisualizationDataUpdated);

	connect( _hQPitchCore, &QPitchCore::updateSignalPresence,
		_gt.widget_qosziview, &QOsziView::setPlotEnabled);
	connect( _hQPitchCore, &QPitchCore::updateSignalPresence,
		this, &QPitch::setUpdateEnabled);

	// ** START PORTAUDIO STREAM ** //
	try {
		Q_ASSERT( _hQPitchCore != NULL );
		_hQPitchCore->startStream( sampleFrequency, fftFrameSize );
	} catch ( QPaSoundInputException& e ) {
		e.report( );
	}

	// ** SETUP THE STATUS BAR ** //
	QString device;
	_hQPitchCore->getPortAudioInfo( device );
	_sb_labelDeviceInfo.setText( device );
	_sb_labelDeviceInfo.setIndent( 10 );
	_gt.statusbar->addWidget( &_sb_labelDeviceInfo, 1 );

	// ** REMOVE MAXIMIZE BUTTON ** //
	Qt::WindowFlags flags = windowFlags( );
	flags &= ~Qt::WindowMaximizeButtonHint;
	setWindowFlags( flags );
}


QPitch::~QPitch( )
{
	// ** ENSURE THAT THE DATA ARE VALID ** //
	Q_ASSERT( _hQPitchCore	!= NULL );
}


void QPitch::setUpdateEnabled( bool enabled )
{
	// ** STORE UPDATE STATUS ** //
	_lineEditEnabled = enabled;
}


void QPitch::closeEvent( QCloseEvent* /* event */ )
{
	// ** ENSURE THAT THE DATA ARE VALID ** //
	Q_ASSERT( _hQPitchCore	!= NULL );

	// ** STORE SETTINGS ** //
	QSettings settings( "QPitch", "QPitch" );

	// audio settings
	QPitchParameters param;
	_hQPitchCore->getStreamParameters( param.sampleFrequency, param.fftFrameSize );
	// _gt.widget_qlogview->getTuningParameters( param.fundamentalFrequency, param.tuningNotation );

	settings.setValue( "audio/samplefrequency", param.sampleFrequency );
	settings.setValue( "audio/buffersize", param.fftFrameSize );
	settings.setValue( "audio/fundamentalfrequency", param.fundamentalFrequency );
	settings.setValue( "audio/tuningnotation", (int)param.tuningNotation );

	// ** STOP THE INPUT STREAM ** //
	try {
		_hQPitchCore->stopStream( );
	} catch ( QPaSoundInputException& e ) {
		e.report( );
	}

}


bool QPitch::eventFilter( QObject* watched, QEvent* event )
{
	if ( ( (watched == _gt.lineEdit_note) || (watched == _gt.lineEdit_frequency) ) &&
		( (event->type( ) >= QEvent::MouseButtonPress) && (event->type( ) <= QEvent::MouseMove) ) ) {
    	// ignore event
		return true;
    } else {
    	// standard event processing
    	return QObject::eventFilter( watched, event );
    }
}



void QPitch::showPreferencesDialog( )
{
	// ** ENSURE THAT THE DATA ARE VALID ** //
	Q_ASSERT( _hQPitchCore	!= NULL );

	// ** GET CURRENT PROPERTIES ** //
	QPitchParameters param;
	_hQPitchCore->getStreamParameters( param.sampleFrequency, param.fftFrameSize );
	// _gt.widget_qlogview->getTuningParameters( param.fundamentalFrequency, param.tuningNotation );

	// ** SHOW PREFERENCES DIALOG ** //
	QSettingsDlg as( param, this );
	connect( &as, &QSettingsDlg::updateApplicationSettings,
		this, &QPitch::setApplicationSettings);
	as.exec( );
}


void QPitch::setApplicationSettings( unsigned int sampleFrequency, unsigned int fftFrameSize,
	double fundamentalFrequency, unsigned int tuningNotation )
{
	// ** UPDATE AUDIO STREAM ** //
	try {
		// ** RESTART THE INPUT STREAM ** //
		Q_ASSERT( _hQPitchCore != NULL );
		_hQPitchCore->stopStream( );
		_hQPitchCore->startStream( sampleFrequency, fftFrameSize );
	} catch ( QPaSoundInputException& e ) {
		e.report( );
	}

	// ** UPDATE NOTE SCALE ** //
	_tuningParameters->setParameters(fundamentalFrequency, (TuningNotation) tuningNotation );
}



void QPitch::showAboutDialog( )
{
	// ** SHOW ABOUT DIALOG ** //
	QAboutDlg ab( this );
	ab.exec( );
}


void QPitch::setViewCompactMode( bool enabled )
{
	// ** MANAGE COMPACT MODE ** //
	if ( enabled == true ) {
		setMinimumSize(800, 600 - _gt.widget_qosziview->height( ) - 6 );
		setMaximumSize(800, 600 - _gt.widget_qosziview->height( ) - 6 );
		_gt.widget_qosziview->setVisible( false );
		_compactModeActivated = true;
	} else {
		_gt.widget_qosziview->setVisible( true );
		setMinimumSize(800, 600);
		setMaximumSize(800, 600);
	}
}

void QPitch::updateQPitchGui( )
{
	static FPSProfiler fp("updateQPitchGui");
	fp.tick();

	// ** UPDATE WIDGETS ** //
	_gt.widget_qosziview->update( );
	_gt.widget_qlogview->update( );
	_gt.widget_freqDiff->update();

	if ( _lineEditEnabled == true ) {
		// ** UPDATE LABELS ** //
		if (_estimatedNote) {
			const EstimatedNote &estimatedNote = _estimatedNote.value();
			_gt.lineEdit_note->setText( QString( "%1 Hz" ).arg( estimatedNote.noteFrequency, 0, 'f', 2 ) );
			_gt.lineEdit_frequency->setText( QString( "%1 Hz" ).arg( estimatedNote.estimatedFrequency, 0, 'f', 2 ) );
			_gt.lineEdit_cents->setText(QString("%1").arg(estimatedNote.currentPitchDeviation * 100.0));
		} else {
			// if frequencies are out of range clear widgets
			_gt.lineEdit_note->clear( );
			_gt.lineEdit_frequency->clear( );
			_gt.lineEdit_cents->clear();
		}
	}

	_gt.lineEdit_fps->setText(QString("%1").arg(fp.get_fps()));

	if ( _compactModeActivated == true ) {
		resize( minimumSize( ) );
		_compactModeActivated = false;
	}
}

void QPitch::onVisualizationDataUpdated(VisualizationData *visData) {
	{
		QMutexLocker visDataLocker(&visData->mutex);
		_gt.widget_qosziview->setPlotSamples(visData->plotSample.data(), visData->timeRangeSample);
		_gt.widget_qosziview->setPlotAutoCorr(visData->plotAutoCorr.data(), visData->estimatedFrequency);
		_gt.widget_qlogview->setEstimatedNote(visData->estimatedNote);
		_estimatedNote = visData->estimatedNote;
		_gt.widget_freqDiff->setEstimatedNote(visData->estimatedNote);
	}
	updateQPitchGui();
}

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
const int QPitch::PLOT_BUFFER_SIZE = 551;       // 44100 * 0.05 / 4 = 551.25
                                                    // size computed to have a time range of 50 ms with an integer downsample ratio
                                                    // sample rate = 44100 Hz --> downsample ratio = 4
                                                    // sample rate = 22050 Hz --> downsample ratio = 2


QPitch::QPitch( QMainWindow* parent ) : QMainWindow( parent )
{
    // ** SETUP THE MAIN WINDOW ** //
    _gt.setupUi( this );

    _settings.load();

    // ** INITIALIZE TUNING PARAMETERS ** //
    _tuningParameters = std::make_shared<TuningParameters>(_settings.fundamentalFrequency, _settings.tuningNotation);

    // ** INTIALIZE QPITCH CORE ** //
    QPitchCoreOptions pitchCoreOptions {
        .sampleFrequency = _settings.sampleFrequency,
        .fftFrameSize = _settings.fftFrameSize,
        .tuningParameters = *_tuningParameters,
    };

    try {
        _hQPitchCore = new QPitchCore(this, PLOT_BUFFER_SIZE, std::move(pitchCoreOptions));
    } catch ( QPaSoundInputException& e ) {
        e.report( );
    }

    // ** INITIALIZE CUSTOM WIDGETS ** //
    _gt.widget_testPlotView->setTitle("Samples [ms] (experimental)");
    _gt.widget_testPlotView->setScaleKind(PlotView::ScaleKind::Linear);
    _gt.widget_testPlotView->setScaleRange(200.0);

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

    connect( _hQPitchCore, &QPitchCore::portAudioStreamStarted,
        this, &QPitch::onPortAudioStreamStarted);

    // ** START THE QPITCH CORE THREAD ** //
    Q_ASSERT( _hQPitchCore != nullptr );
    _hQPitchCore->start();

    // ** REMOVE MAXIMIZE BUTTON ** //
    Qt::WindowFlags flags = windowFlags( );
    flags &= ~Qt::WindowMaximizeButtonHint;
    setWindowFlags( flags );
}


QPitch::~QPitch( )
{
    // ** ENSURE THAT THE DATA ARE VALID ** //
    Q_ASSERT( _hQPitchCore  != nullptr );
}

void QPitch::closeEvent( QCloseEvent* /* event */ )
{
    // ** ENSURE THAT THE DATA ARE VALID ** //
    Q_ASSERT( _hQPitchCore  != nullptr );

    _settings.store();

    // TODO: Ensure the QPitchCore doesn't see the QPitchCore instance destructed.
    _hQPitchCore->requestStop();
}


void QPitch::showPreferencesDialog( )
{
    // ** ENSURE THAT THE DATA ARE VALID ** //
    Q_ASSERT( _hQPitchCore  != nullptr );

    // ** SHOW PREFERENCES DIALOG ** //
    QSettingsDlg as( _settings, this );

    int execResult = as.exec();

    if (execResult == QDialog::Accepted) {
        _settings = as.result();
        setApplicationSettings();
    }
}

void QPitch::setApplicationSettings()
{
    // ** UPDATE NOTE SCALE ** //
    _tuningParameters->setParameters(_settings.fundamentalFrequency, _settings.tuningNotation );

    // ** INTIALIZE QPITCH CORE ** //
    QPitchCoreOptions pitchCoreOptions {
        .sampleFrequency = _settings.sampleFrequency,
        .fftFrameSize = _settings.fftFrameSize,
        .tuningParameters = *_tuningParameters,
    };

    _hQPitchCore->setOptions(std::move(pitchCoreOptions));
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
    _gt.widget_testPlotView->update();
    _gt.widget_qosziview->update( );
    _gt.widget_qlogview->update( );
    _gt.widget_freqDiff->update();

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

    _gt.lineEdit_fps->setText(QString("%1").arg(fp.get_fps()));

    if ( _compactModeActivated == true ) {
        resize( minimumSize( ) );
        _compactModeActivated = false;
    }
}

void QPitch::onVisualizationDataUpdated(VisualizationData *visData) {
    {
        QMutexLocker visDataLocker(&visData->mutex);
        _gt.widget_testPlotView->setData(visData->plotSample);
        _gt.widget_testPlotView->setScaleRange(visData->plotSampleRange);
        _gt.widget_qosziview->setPlotSamples(visData->plotSample.data(), visData->plotSampleRange);
        _gt.widget_qosziview->setPlotSpectrum(visData->plotSpectrum.data());
        _gt.widget_qosziview->setPlotAutoCorr(visData->plotAutoCorr.data(), visData->estimatedFrequency);
        _gt.widget_qlogview->setEstimatedNote(visData->estimatedNote);
        _estimatedNote = visData->estimatedNote;
        _gt.widget_freqDiff->setEstimatedNote(visData->estimatedNote);
    }
    updateQPitchGui();
}

void QPitch::onPortAudioStreamStarted(QString device, QString hostApi) {
    // ** SETUP THE STATUS BAR ** //
    QString msg = QString("Device: %1, Host API: %2").arg(device).arg(hostApi);
    _sb_labelDeviceInfo.setText( msg );
    _sb_labelDeviceInfo.setIndent( 10 );
    _gt.statusbar->addWidget( &_sb_labelDeviceInfo, 1 );
}

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

#include "ui/ui_qpitch.h"

#include "qaboutdlg.h"
#include "qsettingsdlg.h"
#include "qpitchcore.h"
#include "fpsprofiler.h"

#include <QSettings>
#include <QTimer>

// ** CONSTANTS ** //
const int QPitch::PLOT_BUFFER_SIZE = 551; // 44100 * 0.05 / 4 = 551.25
// size computed to have a time range of 50 ms with an integer downsample ratio
// sample rate = 44100 Hz --> downsample ratio = 4
// sample rate = 22050 Hz --> downsample ratio = 2

QPitch::QPitch(QMainWindow *parent) : QMainWindow(parent)
{
    _ui = std::make_unique<Ui::QPitch>();

    // ** SETUP THE MAIN WINDOW ** //
    _ui->setupUi(this);

    _settings.load();

    // ** INITIALIZE TUNING PARAMETERS ** //
    _tuningParameters = std::make_shared<TuningParameters>(_settings.fundamentalFrequency,
                                                           _settings.tuningNotation);

    // ** INTIALIZE QPITCH CORE ** //
    QPitchCoreOptions pitchCoreOptions{
        .sampleFrequency = _settings.sampleFrequency,
        .fftFrameSize = _settings.fftFrameSize,
        .tuningParameters = *_tuningParameters,
    };

    try {
        _hQPitchCore = new QPitchCore(this, PLOT_BUFFER_SIZE, std::move(pitchCoreOptions));
    } catch (QPaSoundInputException &e) {
        e.report();
    }

    // ** INITIALIZE CUSTOM WIDGETS ** //

    // The input signal acquired from the microphone or from the line-in input is plotted in the
    // upper axis. The range of the x-axis is determined by the buffer size of the visualization
    // data, and may vary with the sampling frequency.
    _ui->widget_plotSamples->setTitle("Samples [ms]");
    _ui->widget_plotSamples->setScaleKind(PlotView::ScaleKind::Linear);
    _ui->widget_plotSamples->setLinePen(QPen(Qt::GlobalColor::darkGreen, 1.0));

    // The middle axis shows the energy density spectrum of the input signal in the frequency
    // domain.  It is also the Fourier transform of the autocorrelation.
    _ui->widget_plotSpectrum->setTitle("Frequency Spectrum [Hz]");
    _ui->widget_plotSpectrum->setScaleKind(PlotView::ScaleKind::Linear);
    _ui->widget_plotSpectrum->setLinePen(QPen(Qt::GlobalColor::darkCyan, 1.0));
    _ui->widget_plotSpectrum->setMarkerPen(
            QPen(Qt::GlobalColor::darkYellow, 1.0, Qt::PenStyle::DotLine));

    // The autocorrelation of the input signal is plotted in the lower axis. The x-axis has the same
    // scale as the input signal in the time domain. The peak of the autocorrelation used to detect
    // the frequency of the input signal is indicated by a red line, and its x-coordinate is the
    // period of the input signal, or the reciprocal of the estimated frequency.
    _ui->widget_plotAutoCorr->setTitle("Autocorrelation [ms]");
    _ui->widget_plotAutoCorr->setScaleKind(PlotView::ScaleKind::Linear);
    _ui->widget_plotAutoCorr->setLinePen(QPen(Qt::GlobalColor::darkBlue, 1.0));
    _ui->widget_plotAutoCorr->setMarkerPen(QPen(Qt::GlobalColor::red, 1.0));

    _ui->widget_qlogview->setTuningParameters(_tuningParameters);

    // ** SETUP THE CONNECTIONS ** //
    // File menu
    connect(_ui->action_preferences, &QAction::triggered, this, &QPitch::showPreferencesDialog);

    // Help menu
    connect(_ui->action_about, &QAction::triggered, this, &QPitch::showAboutDialog);
    connect(_ui->action_aboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);

    // Internal connections
    connect(_hQPitchCore, &QPitchCore::visualizationDataUpdated, this,
            &QPitch::onVisualizationDataUpdated);

    connect(_hQPitchCore, &QPitchCore::portAudioStreamStarted, this,
            &QPitch::onPortAudioStreamStarted);

    // ** START THE QPITCH CORE THREAD ** //
    Q_ASSERT(_hQPitchCore != nullptr);
    _hQPitchCore->start();

    // ** REMOVE MAXIMIZE BUTTON ** //
    Qt::WindowFlags flags = windowFlags();
    flags &= ~Qt::WindowMaximizeButtonHint;
    setWindowFlags(flags);
}

QPitch::~QPitch()
{
    // ** ENSURE THAT THE DATA ARE VALID ** //
    Q_ASSERT(_hQPitchCore != nullptr);
}

void QPitch::closeEvent(QCloseEvent * /* event */)
{
    // ** ENSURE THAT THE DATA ARE VALID ** //
    Q_ASSERT(_hQPitchCore != nullptr);

    _settings.store();

    // TODO: Ensure the QPitchCore doesn't see the QPitchCore instance destructed.
    _hQPitchCore->requestStop();
}

void QPitch::showPreferencesDialog()
{
    // ** ENSURE THAT THE DATA ARE VALID ** //
    Q_ASSERT(_hQPitchCore != nullptr);

    // ** SHOW PREFERENCES DIALOG ** //
    QSettingsDlg as(_settings, this);

    int execResult = as.exec();

    if (execResult == QDialog::Accepted) {
        _settings = as.result();
        setApplicationSettings();
    }
}

void QPitch::setApplicationSettings()
{
    // ** UPDATE NOTE SCALE ** //
    _tuningParameters->setParameters(_settings.fundamentalFrequency, _settings.tuningNotation);

    // ** INTIALIZE QPITCH CORE ** //
    QPitchCoreOptions pitchCoreOptions{
        .sampleFrequency = _settings.sampleFrequency,
        .fftFrameSize = _settings.fftFrameSize,
        .tuningParameters = *_tuningParameters,
    };

    _hQPitchCore->setOptions(std::move(pitchCoreOptions));
}

void QPitch::showAboutDialog()
{
    // ** SHOW ABOUT DIALOG ** //
    QAboutDlg ab(this);
    ab.exec();
}

void QPitch::updateQPitchGui()
{
    static FPSProfiler fp("updateQPitchGui");
    fp.tick();

    // ** UPDATE WIDGETS ** //
    _ui->widget_plotSamples->update();
    _ui->widget_plotSpectrum->update();
    _ui->widget_plotAutoCorr->update();
    _ui->widget_qlogview->update();
    _ui->widget_freqDiff->update();

    // ** UPDATE LABELS ** //
    if (_estimatedNote) {
        const EstimatedNote &estimatedNote = _estimatedNote.value();
        _ui->lineEdit_note->setText(QString("%1 Hz").arg(estimatedNote.noteFrequency, 0, 'f', 2));
        _ui->lineEdit_frequency->setText(
                QString("%1 Hz").arg(estimatedNote.estimatedFrequency, 0, 'f', 2));
        _ui->lineEdit_cents->setText(
                QString("%1").arg(estimatedNote.currentPitchDeviation * 100.0));
    } else {
        // if frequencies are out of range clear widgets
        _ui->lineEdit_note->clear();
        _ui->lineEdit_frequency->clear();
        _ui->lineEdit_cents->clear();
    }

    _ui->lineEdit_fps->setText(QString("%1").arg(fp.getFPS()));
}

void QPitch::onVisualizationDataUpdated(VisualizationData *visData)
{
    {
        QMutexLocker visDataLocker(&visData->mutex);

        _ui->widget_plotSamples->setData(visData->plotSample);
        _ui->widget_plotSamples->setScaleRange(visData->plotSampleRange);
        _ui->widget_plotSpectrum->setData(visData->plotSpectrum);
        _ui->widget_plotSpectrum->setScaleRange(visData->plotSpectrumRange);
        _ui->widget_plotSpectrum->setMarker(visData->estimatedFrequency);
        _ui->widget_plotAutoCorr->setData(visData->plotAutoCorr);
        _ui->widget_plotAutoCorr->setScaleRange(visData->plotAutoCorrRange);
        double estimatedPeriod = 1000.0
                / visData->estimatedFrequency; // Period of the detected frequency, in milliseconds.
        _ui->widget_plotAutoCorr->setMarker(estimatedPeriod);

        _ui->widget_qlogview->setEstimatedNote(visData->estimatedNote);
        _estimatedNote = visData->estimatedNote;
        _ui->widget_freqDiff->setEstimatedNote(visData->estimatedNote);
    }
    updateQPitchGui();
}

void QPitch::onPortAudioStreamStarted(QString device, QString hostApi)
{
    // ** SETUP THE STATUS BAR ** //
    QString msg = QString("Device: %1, Host API: %2").arg(device).arg(hostApi);
    _sb_labelDeviceInfo.setText(msg);
    _sb_labelDeviceInfo.setIndent(10);
    _ui->statusbar->addWidget(&_sb_labelDeviceInfo, 1);
}

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

#include "qsettingsdlg.h"

#include <QPushButton>

QSettingsDlg::QSettingsDlg( const QPitchSettings& settings, QWidget* parent ) : QDialog( parent )
{
    // ** SETUP THE MAIN WINDOW ** //
    _sd.setupUi( this );

    // ** SETUP CONNECTIONS ** //
    connect( _sd.buttonBox, &QDialogButtonBox::accepted,
        this, &QSettingsDlg::acceptSettings);
    connect( _sd.buttonBox->button( QDialogButtonBox::RestoreDefaults ), &QPushButton::pressed,
        this, &QSettingsDlg::restoreDefaultSettings);

    load(settings);
}

const QPitchSettings& QSettingsDlg::result()
{
    return _result;
}

void QSettingsDlg::acceptSettings( )
{
    dump(_result);
}


void QSettingsDlg::restoreDefaultSettings( )
{
    QPitchSettings defaultSettings;
    load(defaultSettings);
}

void QSettingsDlg::load(const QPitchSettings &settings)
{
    _sd.comboBox_sampleFrequency->setCurrentIndex( _sd.comboBox_sampleFrequency->findText( QString::number( settings.sampleFrequency ) ) );
    _sd.comboBox_frameSize->setCurrentIndex( _sd.comboBox_frameSize->findText( QString::number( settings.fftFrameSize ) ) );
    _sd.doubleSpinBox_fundamentalFrequency->setValue( settings.fundamentalFrequency );

    switch( settings.tuningNotation ) {
        default:
        case TuningNotation::US:
            _sd.radioButton_scaleUs->setChecked( true );
            break;

        case TuningNotation::FRENCH:
            _sd.radioButton_scaleFrench->setChecked( true );
            break;

        case TuningNotation::GERMAN:
            _sd.radioButton_scaleGerman->setChecked( true );
            break;
    }
}

void QSettingsDlg::dump(QPitchSettings &settings)
{
    // ** UPDATE THE APPLICATION SETTINGS ** //
    settings.sampleFrequency = _sd.comboBox_sampleFrequency->currentText( ).toUInt( );
    settings.fftFrameSize = _sd.comboBox_frameSize->currentText( ).toUInt( );
    settings.fundamentalFrequency = _sd.doubleSpinBox_fundamentalFrequency->value( );

    settings.tuningNotation = TuningNotation::US;

    if ( _sd.radioButton_scaleUs->isChecked( ) ) {
        settings.tuningNotation = TuningNotation::US;
    } else if ( _sd.radioButton_scaleFrench->isChecked( ) ) {
        settings.tuningNotation = TuningNotation::FRENCH;
    } else if ( _sd.radioButton_scaleGerman->isChecked( ) ) {
        settings.tuningNotation = TuningNotation::GERMAN;
    }
}

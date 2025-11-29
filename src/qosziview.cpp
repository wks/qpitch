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

#include "qosziview.h"

#include "fpsprofiler.h"
#include "texthelper.h"

#include <cmath>

#include <QPainter>
#include <iostream>

// ** CUSTOM CONSTANTS ** //
const double    QOsziView::SIDE_MARGIN          = 0.01;
const double    QOsziView::TOP_MARGIN           = 0.055;
const double    QOsziView::AXIS_HALF_HEIGHT     = 0.100;
const int       QOsziView::MINOR_TICK_HEIGHT    = 3;
const int       QOsziView::MIDDLE_TICK_HEIGHT   = 5;
const int       QOsziView::MAJOR_TICK_HEIGHT    = 7;
const int       QOsziView::LABEL_SPACING        = 3;


QOsziView::QOsziView( QWidget* parent ) : QWidget( parent )
{
    // ** SETUP PRIVATE VARIABLES ** //
    _timeRangeSample        = 1.0;                  // dummy values to avoid division by 0

    //** INITIALIZE BUFFERS ** //
    _plotBuffer_size        = 0;                    // empty buffers
}


QOsziView::~QOsziView()
{
    // ** RELEASE RESOURCES ** //
}


void QOsziView::setBufferSize( const unsigned int plotBuffer_size )
{
    //** INITIALIZE BUFFERS ** //
    _plotBuffer_size    = plotBuffer_size;
    _plotSample.clear();
    _plotSample.resize(plotBuffer_size);
    _plotSpectrum.clear();
    _plotSpectrum.resize(plotBuffer_size);
    _plotAutoCorr.clear();
    _plotAutoCorr.resize(plotBuffer_size);
    _estimatedFrequency = 0.0;
}


void QOsziView::setPlotSamples( const double* plotSample, double timeRangeSample )
{
    // ** ENSURE THAT THE BUFFERS ARE VALID ** //
    Q_ASSERT( _plotBuffer_size  != 0 );

    // ** STORE SIGNAL ** //
    memcpy( _plotSample.data(), plotSample, _plotBuffer_size * sizeof( double ) );
    _timeRangeSample = timeRangeSample;
}

void QOsziView::setPlotSpectrum( const double* spectrum )
{
    // ** ENSURE THAT THE BUFFERS ARE VALID ** //
    Q_ASSERT( _plotBuffer_size  != 0 );

    // ** STORE SIGNAL ** //
    memcpy( _plotSpectrum.data(), spectrum, _plotBuffer_size * sizeof( double ) );
}


void QOsziView::setPlotAutoCorr( const double* plotAutoCorr, double estimatedFrequency )
{
    // ** ENSURE THAT THE BUFFERS ARE VALID ** //
    Q_ASSERT( _plotBuffer_size  != 0 );

    // ** STORE AUTOCORRELATION ** //
    memcpy( _plotAutoCorr.data(), plotAutoCorr, _plotBuffer_size * sizeof( double ) );
    _estimatedFrequency = estimatedFrequency;
}


void QOsziView::paintEvent( QPaintEvent* /* event */ )
{
    // ** ENSURE THAT THE BUFFERS ARE VALID ** //
    Q_ASSERT( _plotBuffer_size  != 0 );

    // ** INITIALIZE PAINTER ** //
    QPainter painter;

    // setup the painter
    painter.begin( this );
    painter.setRenderHint(QPainter::Antialiasing);

    // ** COMPUTE DRAWING AREA SIZE ** //
    QRectF rc = rect();
    double width = rc.width();
    double height = rc.height();
    double cellWidth = width;
    double cellHeight = height / 3.0;

    QFont titleFont = painter.font();
    QFontMetricsF titleFontMetrics(titleFont);
    double titleFontHeight = titleFontMetrics.height();

    double plotArea_width      = cellWidth * (1.0 - 2.0 * SIDE_MARGIN);
    double plotArea_sideMargin = cellWidth * SIDE_MARGIN;
    double plotArea_topMargin  = titleFontHeight;
    double plotArea_height     = cellHeight / 2.0 - plotArea_topMargin;

    QPointF titleOffset(plotArea_width / 2.0, -plotArea_height - LABEL_SPACING);

    TextHelper textHelper(painter);

    // ** UPPER AXIS ** //
    painter.resetTransform();
    painter.translate( plotArea_sideMargin, cellHeight * 0 + plotArea_topMargin + plotArea_height );
    drawLinearAxis( painter, plotArea_width, plotArea_height );

    painter.setPen( QPen( palette( ).text( ), 0, Qt::SolidLine ) );
    textHelper.drawTextCenteredUp(titleOffset, "Audio signal [ms]");
    drawCurve( painter, _plotSample.data(), _plotBuffer_size, plotArea_width, plotArea_height, Qt::darkGreen, 0.01 );

    // ** MIDDLE AXIS ** //
    painter.resetTransform();
    painter.translate( plotArea_sideMargin, cellHeight * 1 + plotArea_topMargin + plotArea_height );
    drawLinearAxis( painter, plotArea_width, plotArea_height );

    painter.setPen( QPen( palette( ).text( ), 0, Qt::SolidLine ) );
    textHelper.drawTextCenteredUp(titleOffset, "Frequency spectrum [Hz]");
    drawCurve( painter, _plotSpectrum.data(), _plotBuffer_size, plotArea_width, plotArea_height, Qt::darkCyan, 0 );

    // ** LOWER AXIS ** //
    // move the painter (the horizontal translation is fine)
    painter.resetTransform();
    painter.translate( plotArea_sideMargin, cellHeight * 2 + plotArea_topMargin + plotArea_height );
    drawReversedLogAxis( painter, plotArea_width, plotArea_height );

    painter.setPen( QPen( palette( ).text( ), 0, Qt::SolidLine ) );
    textHelper.drawTextCenteredUp(titleOffset, "Autocorrelation [Hz]");
    drawCurve( painter, _plotAutoCorr.data(), _plotBuffer_size, plotArea_width, plotArea_height, Qt::darkBlue, 0 );

    // draw cursor
    if ( (_estimatedFrequency >= 40.0) && (_estimatedFrequency <= 2000.0) ) {
        painter.setPen( QPen( Qt::red, 0, Qt::SolidLine ) );
        painter.drawLine( QPointF( 40.0 / _estimatedFrequency * plotArea_width, -plotArea_height ),
            QPointF( 40.0 / _estimatedFrequency * plotArea_width, plotArea_height - 1 ) );
    }
}


void QOsziView::drawLinearAxis( QPainter& painter, const double plotArea_width, const double plotArea_height )
{
    // axis range
    const double xAxisRange = 50.0;

    // plot axis
    drawAxisBox( painter, plotArea_width, plotArea_height );

    // plot ticks
    painter.setPen( QPen( palette( ).text( ), 0, Qt::SolidLine ) );

    for ( unsigned int k = 1 ; k < 50 ; ++k ) {
        double xTick = k * 0.02 * plotArea_width;
        double tickHeight = k % 10 == 0 ? MAJOR_TICK_HEIGHT
                          : k % 5  == 0 ? MIDDLE_TICK_HEIGHT
                          : MINOR_TICK_HEIGHT;
        painter.drawLine( QPointF(xTick, plotArea_height), QPointF(xTick, plotArea_height - tickHeight) );
    }

    // plot labels
    painter.save( );
    QFont scaleFont = painter.font( );
    scaleFont.setPointSize( scaleFont.pointSize( ) - 2 );
    painter.setFont( scaleFont );

    TextHelper textHelper(painter);

    painter.setPen( QPen( palette( ).text( ), 0, Qt::SolidLine ) );
    for ( unsigned int k = 0 ; k <= 10 ; ++k ) {
        QPointF textPoint(k * 0.1 * plotArea_width, plotArea_height + LABEL_SPACING);
        textHelper.drawTextCenteredDown(textPoint, QString("%1").arg( (unsigned int)(k * 0.1 * xAxisRange) ));
    }
    painter.restore( );
}


void QOsziView::drawReversedLogAxis( QPainter& painter, const double plotArea_width, const double plotArea_height )
{
    // plot axis
    drawAxisBox( painter, plotArea_width, plotArea_height );

    // plot ticks
    painter.setPen( QPen( palette( ).text( ), 0, Qt::SolidLine ) );

    double  xTick;
    double  freq = 10.0;

    painter.save( );
    QFont scaleFont = painter.font( );
    scaleFont.setPointSize( scaleFont.pointSize( ) - 2 );
    painter.setFont( scaleFont );

    TextHelper textHelper(painter);

    for ( unsigned int k = 5 ; k <= 20 ; ++k ) {
        if ( (k % 10) == 0 ) {
            // move to next decade
            freq *= 10;
            xTick = 40.0 * plotArea_width / freq;
        } else {
            xTick = 40.0 * plotArea_width / ((k % 10) * freq);
        }

        double tickHeight = k % 10 == 0 ? MAJOR_TICK_HEIGHT
                          : k % 5  == 0 ? MIDDLE_TICK_HEIGHT
                          : MINOR_TICK_HEIGHT;
        painter.drawLine( QPointF(xTick, plotArea_height), QPointF(xTick, plotArea_height - tickHeight) );

        QPointF textPoint(xTick, plotArea_height + LABEL_SPACING);

        if ( (k % 10) == 0 ) {
            textHelper.drawTextCenteredDown(textPoint, QString("%1").arg( ( (unsigned int) freq ) ));
        } else if ( (k % 10) == 5 ) {
            textHelper.drawTextCenteredDown(textPoint, QString("%1").arg( ( (unsigned int)((k % 10) * freq) ) ));
        }
    }

    painter.restore();
}


void QOsziView::drawAxisBox( QPainter& painter, const int plotArea_width, const int plotArea_height )
{
    // disable antialiasing and plot a light rectangle
    painter.fillRect( 0, -plotArea_height, plotArea_width, 2 * plotArea_height, palette( ).light( ) );

    // plot the x-axis
    painter.setPen( QPen( palette( ).dark( ), 0, Qt::DashLine ) );
    painter.drawLine( 0, 0, plotArea_width - 1, 0 );

    // plot axis box (slightly bigger than required so it is possible to clean only the inside of the box)
    painter.setPen( QPen( palette( ).dark( ), 0, Qt::SolidLine ) );
    painter.drawRect( 0, -plotArea_height - 1, plotArea_width - 1, 2 * plotArea_height + 1);
}


void QOsziView::drawCurve( QPainter& painter, const double* plotData, const unsigned int plotData_size,
    const int plotArea_width, const int plotArea_height, const QColor& color,
    const double autoScaleThreshold )
{
    // ** ENSURE THAT THE DATA ARE VALID ** //
    Q_ASSERT( plotData != nullptr );
    Q_ASSERT( plotData_size != 0 );

    // enable antialiasing and plot signal samples
    painter.setPen( QPen( color, 0, Qt::SolidLine ) );

    // find min and max of the signal in order to autoscale the signal up to 16 times
    double minValue = plotData[0];
    double maxValue = plotData[0];

    for ( unsigned int k = 1 ; k < plotData_size ; ++k ) {
        if ( plotData[k] < minValue ) {
            minValue = plotData[k];
        }
        if ( plotData[k] > maxValue ) {
            maxValue = plotData[k];
        }
    }

    // find the actual limit value and disable plot if it is below a given threshold
    double limitValue   = (fabs(maxValue) > fabs(minValue)) ? fabs(maxValue) : fabs(minValue);

    // y-axis is upside-down so use a negative scale factor to mirror the plot
    double scaleFactor  = -(0.95 * plotArea_height) / ((limitValue > autoScaleThreshold) ? limitValue : autoScaleThreshold );

    double xIntervalStep = (double) plotArea_width / (double) plotData_size;
    for ( unsigned int k = 1 ; k < plotData_size ; ++k ) {
        painter.drawLine( QPointF( (k - 1) * xIntervalStep, plotData[k - 1] * scaleFactor ),
            QPointF( k * xIntervalStep, plotData[k] * scaleFactor) );
    }
}

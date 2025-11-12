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


#include "qlogview.h"

#include "fpsprofiler.h"

#include <cmath>

#include <QPainter>
#include <QPainterPath>

// ** WIDGET SIZES ** //
const double	QLogView::SIDE_MARGIN			= 0.02;
const int		QLogView::BAR_HEIGHT			= 8;
const int		QLogView::MINOR_TICK_HEIGHT		= 3;
const int		QLogView::MIDDLE_TICK_HEIGHT	= 5;
const int		QLogView::MAJOR_TICK_HEIGHT		= 7;
const int		QLogView::LABEL_OFFSET			= 12;
const int		QLogView::CARET_BORDER			= 5;
const int		QLogView::CURSOR_WIDTH			= 6;
const double	QLogView::ACCEPTED_DEVIATION	= 0.025;


QLogView::QLogView( QWidget* parent ) : QWidget( parent )
{
	// redraw everything the first time and disable cursor
	_drawBackground			= true;
	_drawForeground			= false;
}


void QLogView::setTuningParameters(std::shared_ptr<TuningParameters> tuningParameters)
{
	_tuningParameters = tuningParameters;

	// ** UPDATE THE GUI ** //
	_drawBackground = true;
	update( );
}

void QLogView::setEstimatedNote(std::optional<EstimatedNote> estimatedNote)
{
	_estimatedNote = estimatedNote;
}

void QLogView::setPlotEnabled( bool enabled )
{
	// ** SET THE ACTIVAITON STATUS ** //
	_drawForeground = enabled;
}


void QLogView::paintEvent( QPaintEvent* /* event */ )
{
	Q_ASSERT(_tuningParameters);

	// ** INITIALIZE PAINTER ** //
	QPainter	painter;
	int			scaleWidth = (int)(width( ) * (1.0 - 2 * SIDE_MARGIN));

	/*
	 * redraw the background only when the widget is resized or when
	 * the application is hidden and then shown, using a pixmap to
	 * store the background and to redraw it the next time
	 */

	// ** DRAW THE OFFSCREEN BUFFER ** //
	if ( _drawBackground == true ) {
		// do not redraw background next time
		_drawBackground	= false;

		// create a new picture.
		_picture = QPicture();

		// setup the painter
		painter.begin( &_picture );
    	painter.setRenderHint( QPainter::Antialiasing );
		painter.translate( QPoint( (int)(width( ) * SIDE_MARGIN), height( ) / 2 ) );

		// plot axis frame
		painter.setPen( QPen( palette( ).dark( ), 0, Qt::SolidLine ) );
		painter.drawRect( 0, -BAR_HEIGHT, scaleWidth, 2 * BAR_HEIGHT );
		painter.fillRect( 1, -BAR_HEIGHT + 1, scaleWidth - 1, 2 * BAR_HEIGHT - 1, palette( ).light( ) );

		// plot ticks
		int xTick;
		int xTick_height;

		for ( unsigned int k = 0 ; k <= 120 ; ++k ) {
			xTick = (unsigned int) ( scaleWidth / 120.0 * k );

			if ( ((k+5) % 10) == 0 ) {
				xTick_height = MAJOR_TICK_HEIGHT;
			} else if ( ((k+5) % 5) == 0 ) {
				xTick_height = MIDDLE_TICK_HEIGHT;
			} else {
				xTick_height = MINOR_TICK_HEIGHT;
			}

			painter.drawLine( xTick, -BAR_HEIGHT, xTick, -BAR_HEIGHT - xTick_height );	// upper tick
			painter.drawLine( xTick,  BAR_HEIGHT, xTick,  BAR_HEIGHT + xTick_height );	// lower tick
		}

		// increase the font size
		QFont font = painter.font( );
		font.setPointSize( font.pointSize( ) + 2 );
		painter.setFont( font );

		// plot labels
		painter.setPen( QPen( palette( ).text( ), 0, Qt::SolidLine ) );
		for ( unsigned int k = 0 ; k < 12 ; ++k ) {
			xTick = (int) ( scaleWidth / 24.0 + scaleWidth / 12.0 * k );
			// label above the bar
			const QString &labelAbove = _tuningParameters->getNoteLabel(k, false);
			const QString &labelBelow = _tuningParameters->getNoteLabel(k, true);
			painter.drawText( xTick - (painter.fontMetrics( ).horizontalAdvance(labelAbove) / 2 + 1),
				- BAR_HEIGHT - painter.fontMetrics( ).descent( ) - LABEL_OFFSET,
				labelAbove);
			// label below the bar
			painter.drawText( xTick - (painter.fontMetrics( ).horizontalAdvance(labelBelow) / 2 + 1),
				BAR_HEIGHT + painter.fontMetrics( ).ascent( ) + LABEL_OFFSET,
				labelBelow);
		}
		painter.end( );
	}

	// ** DISPLAY THE OFFSCREEN BUFFER ** //
	painter.begin( this );
    painter.setRenderHint( QPainter::Antialiasing );
	painter.drawPicture( 0, 0, _picture );

	if ( (_drawForeground == true) && _estimatedNote ) {
		const EstimatedNote& estimatedNote = _estimatedNote.value();
		// ** DRAW THE CURSOR IF REQUIRED ** //
		// draw labels
		painter.translate( QPoint( (int)(width( ) * SIDE_MARGIN), height( ) / 2 ) );

		// increase the font size
		QFont font = painter.font( );
		font.setPointSize( font.pointSize( ) + 2 );
		painter.setFont( font );

		int xTick = (int) ( scaleWidth / 24.0 + scaleWidth / 12.0 * estimatedNote.currentPitch );

		const QString &labelAbove = _tuningParameters->getNoteLabel(estimatedNote.currentPitch, false);
		const QString &labelBelow = _tuningParameters->getNoteLabel(estimatedNote.currentPitch, true);

		if ( fabs( estimatedNote.currentPitchDeviation ) < ACCEPTED_DEVIATION ) {
			// draw a square around the note when the error pitch is less than 2.5 percent
			painter.setPen( QPen( Qt::red, 0, Qt::SolidLine ) );
			painter.drawRoundedRect( QRectF ( xTick - painter.fontMetrics( ).horizontalAdvance( labelAbove ) / 2.0 - CARET_BORDER,
				-BAR_HEIGHT - painter.fontMetrics( ).ascent( ) - LABEL_OFFSET - CARET_BORDER,
				painter.fontMetrics( ).horizontalAdvance( labelAbove ) + 2 * CARET_BORDER,
				2 * ( BAR_HEIGHT + painter.fontMetrics( ).ascent( ) + LABEL_OFFSET + CARET_BORDER) ), 15, 15, Qt::RelativeSize );
		}

		// highlight the current note
		painter.setPen( QPen( Qt::red, 0, Qt::SolidLine ) );

		// label above the bar
		painter.drawText( xTick - (painter.fontMetrics( ).horizontalAdvance( labelAbove ) / 2 + 1),
			-BAR_HEIGHT - painter.fontMetrics( ).descent( ) - LABEL_OFFSET,
			labelAbove );
		// label below the bar
		painter.drawText( xTick - (painter.fontMetrics( ).horizontalAdvance( labelBelow ) / 2 + 1),
			BAR_HEIGHT + painter.fontMetrics( ).ascent( ) + LABEL_OFFSET,
			labelBelow );

		// draw the cursor
		painter.setPen( QPen( palette( ).text( ), 0, Qt::SolidLine ) );
		int xCursor = (int) ( scaleWidth / 24.0 + scaleWidth / 12.0 * ( estimatedNote.currentPitch + estimatedNote.currentPitchDeviation ) );
		QColor cursorColor( (int)(0xAA + 0x55 * (1.0 - 2.0 * fabs(estimatedNote.currentPitchDeviation))), 0x00, 0x00 );

		if ( estimatedNote.currentPitchDeviation < -ACCEPTED_DEVIATION ) {
			// draw a right arrow if the pitch is lower than the reference
			QPolygon rightArrow;
			rightArrow << QPoint( xCursor - CURSOR_WIDTH / 2, -BAR_HEIGHT + 1)
				<< QPoint( xCursor - CURSOR_WIDTH / 2, BAR_HEIGHT)
				<< QPoint( xCursor + CURSOR_WIDTH / 2, 0)
				<< QPoint( xCursor - CURSOR_WIDTH / 2, -BAR_HEIGHT + 1);

			QPainterPath path;
			path.addPolygon( rightArrow );

			painter.fillPath( path, cursorColor );
			painter.drawPath( path );
		} else if ( estimatedNote.currentPitchDeviation > ACCEPTED_DEVIATION ) {
			// draw a left arrow if the pitch is lower than the reference
			QPolygon rightArrow;
			rightArrow << QPoint( xCursor + CURSOR_WIDTH / 2, -BAR_HEIGHT + 1)
				<< QPoint( xCursor + CURSOR_WIDTH / 2, BAR_HEIGHT)
				<< QPoint( xCursor - CURSOR_WIDTH / 2, 0)
				<< QPoint( xCursor + CURSOR_WIDTH / 2, -BAR_HEIGHT + 1);

			QPainterPath path;
			path.addPolygon( rightArrow );

			painter.fillPath( path, cursorColor );
			painter.drawPath( path );
		} else {
			// draw a rectangular cursor
			painter.fillRect( xCursor - CURSOR_WIDTH / 2, -BAR_HEIGHT + 1, CURSOR_WIDTH, 2 * BAR_HEIGHT - 1, cursorColor );
			painter.drawRect( xCursor - CURSOR_WIDTH / 2, -BAR_HEIGHT + 1, CURSOR_WIDTH, 2 * BAR_HEIGHT - 2 );
			painter.drawLine( xCursor, -BAR_HEIGHT + 1, xCursor, BAR_HEIGHT - 1 );
		}
	}
}


void QLogView::resizeEvent( QResizeEvent* /* event */ )
{
	// ** REQUEST A BACKGROUND REPAINT ** //
	_drawBackground = true;
}

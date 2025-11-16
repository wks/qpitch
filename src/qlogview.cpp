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
const double	QLogView::BAR_HEIGHT			= 8.0;
const double	QLogView::MINOR_TICK_HEIGHT		= 3.0;
const double	QLogView::MIDDLE_TICK_HEIGHT	= 5.0;
const double	QLogView::MAJOR_TICK_HEIGHT		= 7.0;
const double	QLogView::LABEL_OFFSET			= 12.0;
const double	QLogView::CARET_BORDER			= 5.0;
const double	QLogView::CURSOR_WIDTH			= 6.0;
const double	QLogView::CURSOR_HEIGHT			= QLogView::BAR_HEIGHT - 1.0;
const double	QLogView::ACCEPTED_DEVIATION	= 0.025;

QLogView::QLogView( QWidget* parent ) : QWidget( parent )
{
}


void QLogView::setTuningParameters(std::shared_ptr<TuningParameters> tuningParameters)
{
	_tuningParameters = tuningParameters;
	update( );
}

void QLogView::setEstimatedNote(std::optional<EstimatedNote> estimatedNote)
{
	_estimatedNote = estimatedNote;
}

void QLogView::paintEvent( QPaintEvent* /* event */ )
{
	Q_ASSERT(_tuningParameters);

	// ** INITIALIZE PAINTER ** //
	QPainter	painter;

	// ** Prepare some common properties. ** //

	// Apply the side margin size.
	QRectF widgetRect = rect();
	QPointF center = rect().center();
	QMarginsF margins(SIDE_MARGIN, 0.0, SIDE_MARGIN, 0.0);
	QRectF marginedRect = widgetRect.marginsRemoved(margins);
	double scaleWidth = marginedRect.width();

	// setup the painter
	painter.begin(this);
	painter.setRenderHint( QPainter::Antialiasing );

	painter.translate(QPointF(marginedRect.left(), center.y()));

	QRectF barRect(QPointF(0.0, -BAR_HEIGHT), QSizeF(scaleWidth, 2.0 * BAR_HEIGHT));

	// ** DRAW THE BAR AND THE TICKS ** //

	// plot axis frame
	painter.setPen(QPen(palette().windowText(), 0, Qt::SolidLine));
	painter.setBrush(palette().base());
	painter.drawRect(barRect);

	// plot ticks
	for ( unsigned int k = 0 ; k <= 120 ; ++k ) {
		double xTick = scaleWidth / 120.0 * k;
		double tickHeight;

		if ((k + 5) % 10 == 0) {
			tickHeight = MAJOR_TICK_HEIGHT;
		} else if ((k + 5) % 5== 0) {
			tickHeight = MIDDLE_TICK_HEIGHT;
		} else {
			tickHeight = MINOR_TICK_HEIGHT;
		}

		painter.drawLine( xTick, -BAR_HEIGHT, xTick, -BAR_HEIGHT - tickHeight );	// upper tick
		painter.drawLine( xTick,  BAR_HEIGHT, xTick,  BAR_HEIGHT + tickHeight );	// lower tick
	}

	// ** DISPLAY THE NOTE LABELS ** //

	// Increase the font size for the labels.
	QFont labelFont = font();
	labelFont.setPointSizeF(labelFont.pointSizeF() + 2.0);
	QFontMetricsF fontMetrics(labelFont, this);
	painter.setFont(labelFont);

	// Returns the vector from the text position to the top-mid point.
	auto toTopMiddle = [&](const QString &text) {
		double hr = fontMetrics.horizontalAdvance(text);
		double asc = fontMetrics.ascent();
		return QPointF(hr / 2.0, -asc);
	};

	// Returns the vector from the text position to the bottom-mid point.
	auto toBottomMiddle = [&](const QString &text) {
		double hr = fontMetrics.horizontalAdvance(text);
		double desc = fontMetrics.descent();
		return QPointF(hr / 2.0, desc);
	};

	// plot labels
	QPen penLabel(palette().windowText().color());
	// TODO: Automatically pick a color or let the user pick one,
	// and ensure it contrasts well with both palette().base() and palette().windowText().
	QPen penActiveLabel(QColorConstants::Red);

	for ( unsigned int k = 0 ; k < 12 ; ++k ) {
		double xTick = scaleWidth / 24.0 + scaleWidth / 12.0 * k;
		const QString &labelAbove = _tuningParameters->getNoteLabel(k, false);
		const QString &labelBelow = _tuningParameters->getNoteLabel(k, true);
		painter.setPen(_estimatedNote && k == _estimatedNote->currentPitch ? penActiveLabel : penLabel);
		// label above the bar
		painter.drawText(QPointF(xTick, -BAR_HEIGHT - LABEL_OFFSET) - toBottomMiddle(labelAbove), labelAbove);
		// label below the bar
		painter.drawText(QPointF(xTick, BAR_HEIGHT + LABEL_OFFSET) - toTopMiddle(labelBelow), labelBelow);
	}

	// ** DRAW THE CURSOR IF REQUIRED ** //

	if ( _estimatedNote ) {
		const EstimatedNote& estimatedNote = _estimatedNote.value();

		// draw the cursor
		painter.setPen( QPen( palette( ).text( ), 1.0, Qt::SolidLine ) );
		double xCursor = scaleWidth / 24.0 + scaleWidth / 12.0 * ( estimatedNote.currentPitch + estimatedNote.currentPitchDeviation );
		// TODO: Make sure this color contrasts well against palette().base()
		QColor cursorColor( (int)(0xAA + 0x55 * (1.0 - 2.0 * fabs(estimatedNote.currentPitchDeviation))), 0x00, 0x00 );
		painter.setBrush(cursorColor);

		double cursorHeight = BAR_HEIGHT - 1;

		if ( estimatedNote.currentPitchDeviation < -ACCEPTED_DEVIATION ) {
			// draw a right arrow if the pitch is lower than the reference
			QPolygonF rightArrow;
			rightArrow << QPointF( xCursor - CURSOR_WIDTH / 2, -CURSOR_HEIGHT)
				<< QPointF( xCursor - CURSOR_WIDTH / 2, BAR_HEIGHT)
				<< QPointF( xCursor + CURSOR_WIDTH / 2, 0)
				<< QPointF( xCursor - CURSOR_WIDTH / 2, -CURSOR_HEIGHT);
			painter.drawPolygon(rightArrow);
		} else if ( estimatedNote.currentPitchDeviation > ACCEPTED_DEVIATION ) {
			// draw a left arrow if the pitch is higher than the reference
			QPolygonF leftArrow;
			leftArrow << QPointF( xCursor + CURSOR_WIDTH / 2, -CURSOR_HEIGHT)
				<< QPointF( xCursor + CURSOR_WIDTH / 2, BAR_HEIGHT)
				<< QPointF( xCursor - CURSOR_WIDTH / 2, 0)
				<< QPointF( xCursor + CURSOR_WIDTH / 2, -CURSOR_HEIGHT);
			painter.drawPolygon(leftArrow);
		} else {
			// draw a rectangular cursor
			painter.drawRect(QRectF(QPointF(xCursor - CURSOR_WIDTH / 2, -CURSOR_HEIGHT), QSizeF(CURSOR_WIDTH, 2 * CURSOR_HEIGHT)));
			painter.drawLine(QPointF(xCursor, -CURSOR_HEIGHT), QPointF(xCursor, CURSOR_HEIGHT));
		}
	}
}

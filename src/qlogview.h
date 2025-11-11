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

//! Pitch detection algorithm and note scale visualization.
/*!
 * This class implements a simple pitch detection algorithm to
 * identify the note corresponding to the frequency estimated as
 * the first peak of the autocorrelation function.
 * A linear note scale is displayed using the chosen musical
 * notation. A moving cursor gives a rough indication of the
 * detected note. The identified note is highlighted and
 * when the pitch deviation is smaller than 2.5% a red square
 * is drawn around the note label.
 */

#ifndef __QLOGVIEW_H_
#define __QLOGVIEW_H_

#include "notes.h"

#include <QWidget>
#include <QPicture>

class QLogView : public QWidget {
	Q_OBJECT

public: /* enumerations */
	//! Enumeration of the available tuning scales


public: /* methods */
	//! Default constructor.
	/*!
	 * \param[in] parent handle to the parent widget
	 */
	QLogView( QWidget* parent = 0 );

	//! Set the TuningParameters object
	void setTuningParameters(std::shared_ptr<TuningParameters> tuningParameters);

public slots:
	//! Set the estimated note.
	void setEstimatedNote(std::optional<EstimatedNote> estimatedNote);

	//! Enable the visualization of the cursor on the note scale.
	/*!
	 * \param[in] enabled the status of the cursor
	 */
	void setPlotEnabled( bool enabled );

protected: /* methods */
	//! Function called to handle a repaint request.
	/*!
	 * \param[in] event the details of the repaint event
	 */
	virtual void paintEvent( QPaintEvent* event );

	//! Function called to handle a resize request.
	/*!
	 * \param[in] event the details of the resize event
	 */
	virtual void resizeEvent( QResizeEvent* event );


private: /* static constants */
	// ** WIDGETS SIZES ** //
	static const double	SIDE_MARGIN;					//!< Percent width of the horizontal border
	static const int	BAR_HEIGHT;						//!< Half the height of the tuning bar
	static const int	MINOR_TICK_HEIGHT;				//!< Height of the minor ticks
	static const int	MIDDLE_TICK_HEIGHT;				//!< Height of the middle ticks
	static const int	MAJOR_TICK_HEIGHT;				//!< Height of the major ticks
	static const int	LABEL_OFFSET;					//!< Distance of the labels from the tuning bar
	static const int	CARET_BORDER;					//!< Space between the label and the rounded rectangle displayed when the error is below 2.5 percent
	static const int	CURSOR_WIDTH;					//!< Width of the cursor displayed in the tuning bar
	static const double	ACCEPTED_DEVIATION;				//!< Pitch deviation where the cursor becomes a rectangle


private: /* members */
	// ** TUNING NOTATIONS ** //
	static const QString NoteLabel[6][12];				//!< Labels of the note in different tuning scales

	// ** PITCH DETECTION PARAMETERS ** //
	std::shared_ptr<TuningParameters>	_tuningParameters;			//!< Tuning parameters
	std::optional<EstimatedNote>		_estimatedNote;

	// ** REPAINT FLAG **//
	bool				_drawBackground;				//!< Redraw everything when true, otherwise redraw only the note scale
	bool				_drawForeground;				//!< Draw the note cursor when true, otherwise draw nothing
	QPicture			_picture;						//!< QPicture used to store the background to reduce the load
};

#endif /* __QLOGVIEW_H_ */

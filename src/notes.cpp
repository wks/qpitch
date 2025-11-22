#include "notes.h"

#include <QString>

// ** MUSICAL NOTATIONS ** //
const QString NoteLabel[6][12] = {
    {  "A",  QString("A%1").arg(QChar(0x266F)),  "B",  "C",  QString("C%1").arg(QChar(0x266F)),  "D",  QString("D%1").arg(QChar(0x266F)),  "E",  "F",   QString("F%1").arg(QChar(0x266F)),   "G",   QString("G%1").arg(QChar(0x266F)) },    /* US  */
    {  "A",  QString("B%1").arg(QChar(0x266D)),  "B",  "C",  QString("D%1").arg(QChar(0x266D)),  "D",  QString("E%1").arg(QChar(0x266D)),  "E",  "F",   QString("G%1").arg(QChar(0x266D)),   "G",   QString("A%1").arg(QChar(0x266D)) },    /* US alternate */
    { "La", QString("La%1").arg(QChar(0x266F)), "Si", "Do", QString("Do%1").arg(QChar(0x266F)), "Re", QString("Re%1").arg(QChar(0x266F)), "Mi", "Fa",  QString("Fa%1").arg(QChar(0x266F)), "Sol", QString("Sol%1").arg(QChar(0x266F)) },    /* French */
    { "La", QString("Si%1").arg(QChar(0x266D)), "Si", "Do", QString("Re%1").arg(QChar(0x266D)), "Re", QString("Mi%1").arg(QChar(0x266D)), "Mi", "Fa", QString("Sol%1").arg(QChar(0x266D)), "Sol",  QString("La%1").arg(QChar(0x266D)) },    /* French alternate */
    {  "A",                                "B",  "H",  "C",  QString("C%1").arg(QChar(0x266F)),  "D",  QString("D%1").arg(QChar(0x266F)),  "E",  "F",   QString("F%1").arg(QChar(0x266F)),   "G",   QString("G%1").arg(QChar(0x266F)) },    /* German */
    {  "A",                                "B",  "H",  "C",  QString("D%1").arg(QChar(0x266D)),  "D",  QString("E%1").arg(QChar(0x266D)),  "E",  "F",   QString("G%1").arg(QChar(0x266D)),   "G",   QString("A%1").arg(QChar(0x266D)) }     /* German alternate */
};

// ** NOTE RATIOS ** //
const double    TuningParameters::D_NOTE                = pow( 2.0, 1.0/12.0 );
const double    TuningParameters::D_NOTE_LOG            = log10( pow( 2.0, 1.0/12.0 ) ) / log10( 2.0 );

TuningParameters::TuningParameters(double fundamentalFrequency, TuningNotation tuningNotation) {
    setParameters(fundamentalFrequency, tuningNotation);
}

void TuningParameters::setParameters(double fundamentalFrequency, TuningNotation tuningNotation) {
    _fundamentalFrequency = fundamentalFrequency;
    _tuningNotation = tuningNotation;

    // ** UPDATE PITCH DETECTION CONSTANTS ** //
    for ( unsigned int k = 0 ; k < 12 ; ++k ) {
        _noteFrequency[k]   = _fundamentalFrequency * pow( D_NOTE, (int) k );                       // set frequencies for pitch detection
        _noteScale[k]       = log10( _fundamentalFrequency ) / log10( 2.0 ) + (k * D_NOTE_LOG);     // set frequencies for visualization
    }
}

const QString& TuningParameters::getNoteLabel(int seq, bool alternative) const {
    Q_ASSERT(0 <= seq && seq < 12);
    int row = (int)_tuningNotation * 2 + (int)alternative;
    Q_ASSERT(0 <= row && row < 6);
    return NoteLabel[row][seq];
}

std::optional<EstimatedNote> TuningParameters::estimateNote(const double estimatedFrequency) const {
    // process only notes within the range [40, 2000] Hz
    if (!((estimatedFrequency >= 40.0) && (estimatedFrequency <= 2000.0))) {
        return std::nullopt;
    }

    double octaveNormalizedFrequency = estimatedFrequency;

    // ** ESTIMATE THE NEW PITCH ** //
    // compute the deviation of the estimated frequency with respect to the reference octave
    int octaveDeviation = 0;

    // rescale the estimatedFrequency to bring it inside the reference octave (4th octave)
    while ( octaveNormalizedFrequency > (_noteFrequency[11] + _fundamentalFrequency * D_NOTE_LOG / 2.0 ) ) {
        // higher frequency, higher octave
        octaveNormalizedFrequency /= 2.0;
        octaveDeviation++;
    }

    while ( octaveNormalizedFrequency < (_noteFrequency[0] - _fundamentalFrequency * D_NOTE_LOG / 2.0 ) ) {
        // lower frequency, lower octave
        octaveNormalizedFrequency *= 2.0;
        octaveDeviation--;
    }

    /*
     * here octaveNormalizedFrequency is in the range ( _noteFrequency[0], _noteFrequency[11] )
     * so it is possible to find the pitch in the scale as the one with the minimum
     * distance in the LINEAR scale (logarithm of the frequencies)
     */

    // Note: No.  It is between (_noteFrequency[0] - _fundamentalFrequency * D_NOTE_LOG / 2.0 ) and (_noteFrequency[11] + _fundamentalFrequency * D_NOTE_LOG / 2.0 )
    // Q_ASSERT(octaveNormalizedFrequency >= _noteFrequency[0]);
    // Q_ASSERT(octaveNormalizedFrequency <= _noteFrequency[11]);

    double          minPitchDeviation       = _fundamentalFrequency * D_NOTE_LOG / 2.0;
    unsigned int    minPitchDeviation_index = 0;
    double          log_octaveNormalizedFreq    = log10(octaveNormalizedFrequency) / log10( 2.0 );

    for ( unsigned int k = 0 ; k < 12 ; ++k ) {
        if ( fabs( log_octaveNormalizedFreq - _noteScale[k] ) < minPitchDeviation ) {
            minPitchDeviation       = fabs( log_octaveNormalizedFreq - _noteScale[k] );
            minPitchDeviation_index = k;
        }
    }

    EstimatedNote result;
    result.estimatedFrequency = estimatedFrequency;
    result.currentPitch = minPitchDeviation_index;
    result.currentPitchDeviation    = ( log_octaveNormalizedFreq - _noteScale[minPitchDeviation_index] ) / D_NOTE_LOG;
    result.noteFrequency = _noteFrequency[minPitchDeviation_index] * pow( 2.0, octaveDeviation );

    return result;
}

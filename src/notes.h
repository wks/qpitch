#pragma once

#include <cstdlib>
#include <optional>
#include <QString>

enum class TuningNotation {
    US = 0,
    FRENCH,
    GERMAN,
};

struct EstimatedNote
{
    double estimatedFrequency;

    /// Frequency of the closest note in the scale
    int currentPitch;

    /// percentuale da -0.5 a +0.5 che dice di quanto va disegnato spostato
    double currentPitchDeviation;

    double noteFrequency;
};

class TuningParameters
{
public:
    TuningParameters(double fundamentalFrequency, TuningNotation tuningNotation);

    void setParameters(double fundamentalFrequency, TuningNotation tuningNotation);

    const QString &getNoteLabel(int seq, bool alternative) const;

    /// This method implements a simple pitch detection algorithm to
    /// identify the note corresponding to the frequency estimated as
    /// the first peak of the autocorrelation function.
    std::optional<EstimatedNote> estimateNote(double estimatedFrequency) const;

public:
    /// Ratio of two consecutive notes
    static const double D_NOTE;

    /// Base-2 logarithm of the ratio of two consecutive notes
    static const double D_NOTE_LOG;

private:
    /// Fundamental frequency used as a reference to build the pitch scale
    double _fundamentalFrequency;

    /// Musical notation used to select the string displayed
    TuningNotation _tuningNotation;

    /// Frequencies of the notes in the reference octave used for pitch detection
    double _noteFrequency[12];

    /// Scale of the notes in the reference octave used for visualization
    double _noteScale[12];
};

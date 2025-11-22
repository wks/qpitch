#include "qpitchsettings.h"

#include <QSettings>

QPitchSettings::QPitchSettings() {
    loadDefault();
}

void QPitchSettings::loadDefault() {
    sampleFrequency = 44100;
    fftFrameSize = 4096;
    fundamentalFrequency = 440.0;
    tuningNotation = TuningNotation::US;
}

template<class T, class F>
void loadValidateAndSet(QSettings &settings, QAnyStringView key, T &var, F validate) {
    QVariant value = settings.value(key);
    if (!value.isValid()) {
        return;
    }

    T valueTyped = qvariant_cast<T>(value);
    if (validate(valueTyped)) {
        qInfo() << "Applying setting '" << key << "'.  Value: " << value;
        var = valueTyped;
    } else {
        qInfo() << "Invalid value for setting '" << key << "'.  Value: " << value;
    }
}

void QPitchSettings::load() {
    // ** RETRIEVE APPLICATION SETTINGS ** //
    QSettings settings( "QPitch", "QPitch" );

    loadValidateAndSet(settings, "audio/samplefrequency", sampleFrequency, [](auto v) {
        // restrict sample frequency to 44100 and 22050 Hz
        return v == 44100 || v == 22050;
    });

    loadValidateAndSet(settings, "audio/buffersize", fftFrameSize, [](auto v) {
        // restrict frame buffer size to 8192 and 4096
        return v == 8192 || v == 4096;
    });

    loadValidateAndSet(settings, "audio/fundamentalfrequency", fundamentalFrequency, [](auto v) {
        // restrict the fundamental frequency to the range [400, 480] Hz
        return 400 <= v && v <= 480;
    });

    loadValidateAndSet(settings, "audio/tuningnotation", tuningNotation, [](auto v) {
        // restrict the fundamental TuningNotation to the range 0 (US) - 1 (French) - 2 (German)
        return v <= TuningNotation::GERMAN;
    });
}

template<class T>
void storeSetting(QSettings &settings, QAnyStringView key, T v) {
    QVariant value = v;
    qInfo() << "Storing setting '" << key << "'.  Value: " << value;
    settings.setValue(key, value);
}


void QPitchSettings::store() {
    // ** STORE SETTINGS ** //
    QSettings settings( "QPitch", "QPitch" );

    storeSetting(settings, "audio/samplefrequency", sampleFrequency );
    storeSetting(settings, "audio/buffersize", fftFrameSize );
    storeSetting(settings, "audio/fundamentalfrequency", fundamentalFrequency );
    storeSetting(settings, "audio/tuningnotation", (int)tuningNotation );
}

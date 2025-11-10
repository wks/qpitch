#pragma once

#include <QWidget>
#include <QSize>

class FreqDiffView : public QWidget {
public:
    explicit FreqDiffView(QWidget *parent);

public slots:
    void setSignalPresent(bool signalPresent);
    void setEstimatedFrequency(double setEstimatedFrequency);
    void setEstimatedNote(double estimatedNote);

protected:
    virtual void paintEvent(QPaintEvent *event) override;
    QSize minimumSizeHint() const override;

private:
    bool _signalPresent;
    double _estimatedFrequency;
    double _estimatedNote;
};

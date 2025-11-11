#pragma once

#include "notes.h"

#include <QWidget>
#include <QSize>

class FreqDiffView : public QWidget {
    Q_OBJECT
public:
    explicit FreqDiffView(QWidget *parent);

public slots:
    void setSignalPresent(bool signalPresent);
    void setEstimatedNote(std::optional<EstimatedNote> estimatedNote);

protected:
    virtual void paintEvent(QPaintEvent *event) override;
    QSize minimumSizeHint() const override;

private:
    bool _signalPresent;
    std::optional<EstimatedNote> _estimatedNote;
};

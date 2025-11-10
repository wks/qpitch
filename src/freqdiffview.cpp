#include "freqdiffview.h"

#include <QPainter>

FreqDiffView::FreqDiffView(QWidget *parent): QWidget(parent) {
    _signalPresent = false;
    _estimatedFrequency = 0.0;
    _estimatedNote = 0.0;
}

void FreqDiffView::setSignalPresent(bool signalPresent) {
    _signalPresent = signalPresent;
}

void FreqDiffView::setEstimatedFrequency(double estimatedFrequency) {
    _estimatedFrequency = estimatedFrequency;
}

void FreqDiffView::setEstimatedNote(double estimatedNote) {
    _estimatedNote = estimatedNote;
}

void FreqDiffView::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF rect = QWidget::rect();
    QPointF center = rect.center();

    QBrush brushBg(Qt::BrushStyle::SolidPattern);
    if (_signalPresent) {
        brushBg.setColor(qRgb(255, 255, 128));
    } else {
        brushBg.setColor(qRgb(128, 255, 255));
    }
    painter.setBrush(brushBg);
    painter.drawRect(rect);

    QPen penCenter;
    penCenter.setColor(QColor::fromRgb(qRgb(0, 0, 0)));
    penCenter.setStyle(Qt::PenStyle::SolidLine);
    penCenter.setWidthF(1.0);

    painter.setPen(penCenter);
    painter.drawLine(QPoint(center.x(), rect.top()), QPointF(center.x(), rect.bottom()));

    if (_signalPresent && _estimatedFrequency != 0.0 && _estimatedNote != 0.0) {
        double diffSemitone = (log2(_estimatedFrequency) - log2(_estimatedNote)) * 12.0;
        double clamped = std::clamp(diffSemitone, -0.5, 0.5);
        double estX = center.x() + rect.width() * clamped;

        QPen penEstimated;
        penEstimated.setColor(QColor::fromRgb(qRgb(255, 0, 0)));
        penEstimated.setStyle(Qt::PenStyle::SolidLine);
        penEstimated.setWidthF(1.0);

        painter.setPen(penEstimated);
        painter.drawLine(QPoint(estX, rect.top()), QPointF(estX, rect.bottom()));
    }
}

QSize FreqDiffView::minimumSizeHint() const {
    return QSize(100, 50);
}

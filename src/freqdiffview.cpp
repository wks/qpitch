#include "freqdiffview.h"

#include <QPainter>

FreqDiffView::FreqDiffView(QWidget *parent) : QWidget(parent) { }

void FreqDiffView::setEstimatedNote(std::optional<EstimatedNote> estimatedNote)
{
    _estimatedNote = estimatedNote;
}

void FreqDiffView::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF rect = QWidget::rect();
    QPointF center = rect.center();

    QBrush brushBg(Qt::BrushStyle::SolidPattern);
    brushBg.setColor(qRgb(255, 255, 128));

    painter.setPen(Qt::NoPen);
    painter.setBrush(brushBg);
    painter.drawRect(rect);

    QPen penCenter;
    penCenter.setColor(QColor::fromRgb(qRgb(0, 0, 0)));
    penCenter.setStyle(Qt::PenStyle::SolidLine);
    penCenter.setWidthF(1.0);

    painter.setPen(penCenter);
    painter.drawLine(QPointF(center.x(), rect.top()), QPointF(center.x(), rect.bottom()));

    if (_estimatedNote) {
        const EstimatedNote &estimatedNote = _estimatedNote.value();
        double estX = center.x() + rect.width() * estimatedNote.currentPitchDeviation;

        QPen penEstimated;
        penEstimated.setColor(QColor::fromRgb(qRgb(255, 0, 0)));
        penEstimated.setStyle(Qt::PenStyle::SolidLine);
        penEstimated.setWidthF(1.0);

        painter.setPen(penEstimated);
        painter.drawLine(QPointF(estX, rect.top()), QPointF(estX, rect.bottom()));
    }
}

QSize FreqDiffView::minimumSizeHint() const
{
    return QSize(100, 50);
}

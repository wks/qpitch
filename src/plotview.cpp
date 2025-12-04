#include "plotview.h"

#include "texthelper.h"

#include <QPainter>

const double PlotView::X_MARGIN            = 10;
const double PlotView::Y_MARGIN            = 5;
const double PlotView::LABEL_SPACING       = 3;
const double PlotView::MINOR_TICK_HEIGHT   = 3;
const double PlotView::MIDDLE_TICK_HEIGHT  = 5;
const double PlotView::MAJOR_TICK_HEIGHT   = 7;

PlotView::PlotView(QWidget *parent): QWidget(parent) {
    _title = "No title";
    _scaleKind = ScaleKind::Linear;
    _scaleRange = 0.0;

    _linePen = QPen(Qt::GlobalColor::darkGreen, 1.0);
    _markerPen = QPen(Qt::GlobalColor::red, 1.0);

    _dataPoints.clear();
    _marker = std::nullopt;
}

void PlotView::setTitle(const QString &title) {
    _title = title;
}

void PlotView::setScaleKind(ScaleKind scaleKind) {
    _scaleKind = scaleKind;
}

void PlotView::setScaleRange(double scaleRange) {
    _scaleRange = scaleRange;
}

void PlotView::setLinePen(const QPen &pen) {
    _linePen = pen;
}

void PlotView::setMarkerPen(const QPen &pen) {
    _markerPen = pen;
}

void PlotView::setData(const std::vector<double> &newData) {
    _data = newData;
    _dataPoints.resize(newData.size());
}

void PlotView::setMarker(std::optional<double> marker) {
    _marker = marker;
}

void PlotView::paintEvent(QPaintEvent* event) {
    QPainter painter;

    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // ** FONT-RELATED STUFF ** //
    QFont titleFont = painter.font();
    QFont scaleFont = painter.font();
    scaleFont.setPointSizeF(scaleFont.pointSizeF() - 2.0);

    TextHelper textHelper(painter);

    QFontMetricsF titleFontMetrics(titleFont);
    QFontMetricsF scaleFontMetrics(scaleFont);

    double titleFontHeight = titleFontMetrics.height();
    double scaleFontHeight = scaleFontMetrics.height();

    // ** COMPUTE SIZES ** //
    QRectF rc = rect();
    QRectF marginedRc = rc.marginsRemoved(QMarginsF(X_MARGIN, Y_MARGIN, X_MARGIN, Y_MARGIN));
    QRectF plotAreaRc = marginedRc.marginsRemoved(QMarginsF(
        0.0, titleFontHeight + LABEL_SPACING, 0.0, scaleFontHeight + LABEL_SPACING));

    // ** DRAW BOX ** //
    drawAxisBox(painter, plotAreaRc);

    // ** DRAW AXIS AND SCALES ** //
    drawLinearAxis(painter, plotAreaRc);

    // ** DRAW TITLE ** //
    painter.setPen(QPen(palette().text(), 0, Qt::SolidLine));
    textHelper.drawTextCenteredUp(QPointF(plotAreaRc.center().x(), plotAreaRc.top() - LABEL_SPACING), _title);

    // ** DRAW CURVE ** //
    drawCurve(painter, plotAreaRc, 0.01);

    // ** DRAW MARKER ** //
    if (_marker.has_value()) {
        double markerValue = _marker.value();
        painter.setPen(_markerPen);
        double markerX = std::lerp(plotAreaRc.left(), plotAreaRc.right(), markerValue / _scaleRange);
        painter.drawLine(QPointF(markerX, plotAreaRc.top()), QPointF(markerX, plotAreaRc.bottom()));
    }
}

void PlotView::drawAxisBox(QPainter& painter, const QRectF &rc) {
    // Draw the box with filling
    painter.setPen(QPen(palette().dark(), 1.0, Qt::SolidLine));
    painter.setBrush(palette().light());
    painter.drawRect(rc);

    // plot the x-axis
    painter.setPen(QPen(palette().dark(), 1.0, Qt::DashLine));
    painter.drawLine(QPointF(rc.left(), rc.center().y()), QPointF(rc.right(), rc.center().y()));
}

void PlotView::drawLinearAxis(QPainter& painter, const QRectF &rc) {
    painter.save();
    QFont scaleFont = painter.font();
    scaleFont.setPointSize(scaleFont.pointSize() - 2);
    painter.setFont(scaleFont);

    TextHelper textHelper(painter);

    QPen penTick(palette().dark(), 1.0, Qt::SolidLine);
    QPen penLabel(palette().text().color());

    auto drawTickAndLabel = [&](double xTick, int level, int maxLevel, double value) {
        double tickHeight = level == 0 ? MAJOR_TICK_HEIGHT
                          : level == 1 ? MIDDLE_TICK_HEIGHT
                          : MINOR_TICK_HEIGHT;
        painter.setPen(penTick);
        painter.drawLine(QPointF(xTick, rc.bottom()), QPointF(xTick, rc.bottom() - tickHeight));

        if (level <= maxLevel) {
            QPointF textPoint(xTick, rc.bottom() + LABEL_SPACING);
            painter.setPen(penLabel);
            textHelper.drawTextCenteredDown(textPoint, QString("%1").arg(value));
        }
    };

    // axis range
    const double xAxisRange = _scaleRange;

    if (0 < xAxisRange && xAxisRange < INFINITY) {
        // xAxisRange = b * 10^p for some b where 1 <= b < 10
        double p = std::floor(std::log10(xAxisRange));
        double b1 = pow(10.0, p);

        // Check if 1 <= b < 2
        bool between1And2 = xAxisRange < b1 * 2;

        double majorUnit;
        double maxLevel;
        if (!between1And2) {
            // We usually keep one significant figure for the distance between major ticks.
            // So if xAxisRange = 63, then major ticks will be at 0, 10, 20, 30, 40, 50 and 60.
            majorUnit = b1;
            maxLevel = 1;
        } else {
            // But if b == 1, we only make the major ticks one order of magnitude finer.
            // So if xAxisRange = 13, we place major ticks at 0, 1, 2, 3, 4, ..., 9, 10, 11, 12, 13.
            // In this way, we may have at most 20 major ticks, from 0 to 19.
            majorUnit = b1 / 10.0;
            maxLevel = 0;
        }

        for (int mi = 0; mi < 200; mi++) {
            double value = majorUnit * mi / 10.0;

            if (value > xAxisRange) {
                break;
            }

            int level = mi % 10 == 0 ? 0
                      : mi % 5 == 0 ? 1
                      : 2;
            double xTick = std::lerp(rc.left(), rc.right(), value / xAxisRange);
            drawTickAndLabel(xTick, level, maxLevel, value);
        }
    }

    painter.restore( );
}


void PlotView::drawCurve(QPainter& painter, const QRectF &rc, const double autoScaleThreshold)
{
    // enable antialiasing and plot signal samples
    painter.setPen(_linePen);

    if (_data.empty()) {
        painter.drawLine(rc.topLeft(), rc.bottomRight());
        painter.drawLine(rc.bottomLeft(), rc.topRight());
        return;
    }

    // find min and max of the signal in order to autoscale the signal up to 16 times
    const auto [minValue, maxValue] = std::minmax_element(std::begin(_data), std::end(_data));

    // find the actual limit value and disable plot if it is below a given threshold
    double limitValue = std::max(std::fabs(*maxValue), std::fabs(*minValue));

    // y-axis is upside-down so use a negative scale factor to mirror the plot
    double scaleFactor  = -(0.95 * rc.height() / 2) / std::max(limitValue, autoScaleThreshold);

    double xLeft = rc.left();
    double xRight = rc.right();
    double yMiddle = rc.center().y();
    for (size_t k = 0; k < _dataPoints.size(); k++) {
        _dataPoints[k].setX(std::lerp(xLeft, xRight, (double)k / _dataPoints.size()));
        _dataPoints[k].setY(yMiddle + _data[k] * scaleFactor);
    }

    painter.drawPolyline(_dataPoints.data(), _dataPoints.size());
}

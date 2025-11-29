#include "plotview.h"

#include "texthelper.h"

#include <QPainter>

const double PlotView::X_MARGIN            = 10;
const double PlotView::Y_MARGIN            = 5;
const double PlotView::LABEL_SPACING       = 3;
const double PlotView::MINOR_TICK_HEIGHT   = 3;
const double PlotView::MIDDLE_TICK_HEIGHT  = 5;
const double PlotView::MAJOR_TICK_HEIGHT   = 7;

PlotView::PlotView(QWidget *parent): PlotView(parent, "No title", ScaleKind::Linear, 1000.0) {}

PlotView::PlotView(QWidget *parent, QString title, ScaleKind scaleKind,
                     double scaleRange):
    _title(title),
    _scaleKind(scaleKind),
    _scaleRange(scaleRange)
{
    _data.clear();
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

void PlotView::setData(const std::vector<double> &newData) {
    _data = newData;
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
    drawCurve(painter, plotAreaRc, Qt::darkGreen, 0.01);

    // draw cursor
    if (_marker.has_value()) {
        double markerValue = _marker.value();
        painter.setPen( QPen( Qt::red, 0, Qt::SolidLine ) );
        double markerX = 40.0 / markerValue * plotAreaRc.width();
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
    // axis range
    const double xAxisRange = 50.0;

    // plot ticks
    painter.setPen(QPen(palette().text(), 1.0, Qt::SolidLine ) );

    for (size_t k = 1 ; k < 50 ; ++k) {
        double xTick = rc.left() + k * 0.02 * rc.width();
        double tickHeight = k % 10 == 0 ? MAJOR_TICK_HEIGHT
                          : k % 5  == 0 ? MIDDLE_TICK_HEIGHT
                          : MINOR_TICK_HEIGHT;
        painter.drawLine(QPointF(xTick, rc.bottom()), QPointF(xTick, rc.bottom() - tickHeight));
    }

    // plot labels
    painter.save( );
    QFont scaleFont = painter.font( );
    scaleFont.setPointSize( scaleFont.pointSize( ) - 2 );
    painter.setFont( scaleFont );

    TextHelper textHelper(painter);

    painter.setPen( QPen( palette( ).text( ), 0, Qt::SolidLine ) );
    for ( unsigned int k = 0 ; k <= 10 ; ++k ) {
        QPointF textPoint(rc.left() + k * 0.1 * rc.width(), rc.bottom() + LABEL_SPACING);
        textHelper.drawTextCenteredDown(textPoint, QString("%1").arg( (unsigned int)(k * 0.1 * xAxisRange) ));
    }
    painter.restore( );
}


void PlotView::drawCurve(QPainter& painter, const QRectF &rc, const QColor& color,
    const double autoScaleThreshold)
{
    // enable antialiasing and plot signal samples
    painter.setPen( QPen( color, 0, Qt::SolidLine ) );

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

    double xIntervalStep = rc.width() / _data.size();
    QPointF origin(rc.left(), rc.center().y());
    for (size_t k = 1 ; k < _data.size() ; ++k) {
        painter.drawLine(origin + QPointF((k - 1) * xIntervalStep, _data[k - 1] * scaleFactor),
            origin + QPointF(k * xIntervalStep, _data[k] * scaleFactor));
    }
}

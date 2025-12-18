#pragma once

#include <QString>
#include <QWidget>
#include <QPaintEvent>
#include <QPen>

#include <optional>

/*!
 * This class implements an oscilloscope-like widget for the visualization of signals in the time
 * domain or the frequency domain.
 */
class PlotView : public QWidget
{
    Q_OBJECT

public:
    enum class ScaleKind {
        Linear,
        Logarithmic,
    };

    explicit PlotView(QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setScaleKind(ScaleKind scaleKind);
    void setScaleRange(double scaleRange);
    void setLinePen(const QPen &pen);
    void setMarkerPen(const QPen &pen);
    void setData(const std::vector<double> &newData);
    void setMarker(std::optional<double> marker);

protected:
    virtual void paintEvent(QPaintEvent *event) override;

private:
    static const double X_MARGIN; //!< Horizontal margin in pixels
    static const double Y_MARGIN; //!< Vertical margin in pixels
    static const double LABEL_SPACING; //!< Pixel distance of the labels from the axis
    static const double MINOR_TICK_HEIGHT; //!< Pixel height of the minor ticks
    static const double MIDDLE_TICK_HEIGHT; //!< Pixel height of the minor ticks
    static const double MAJOR_TICK_HEIGHT; //!< Pixel height of the major ticks

    void drawAxisBox(QPainter &painter, const QRectF &rc);
    void drawLinearAxis(QPainter &painter, const QRectF &rc);
    void drawCurve(QPainter &painter, const QRectF &rc, const double autoScaleThreshold);

    QString _title;
    ScaleKind _scaleKind;
    double _scaleRange;
    QPen _linePen;
    QPen _markerPen;
    std::vector<double> _data;
    std::vector<QPointF> _dataPoints;
    std::optional<double> _marker;
};

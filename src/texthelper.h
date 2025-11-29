#pragma once

#include <QPainter>
#include <QFont>

class TextHelper {
public:
    TextHelper(QPainter &painter);

    QFontMetricsF fontMetrics();

    QPointF toTopCenter(const QString &text);
    QPointF toBottomCenter(const QString &text);

    void drawTextCenteredUp(QPointF point, const QString &text);
    void drawTextCenteredDown(QPointF point, const QString &text);

private:
    QPainter &_painter;
};

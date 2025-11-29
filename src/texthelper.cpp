#include "texthelper.h"

TextHelper::TextHelper(QPainter &painter) :
    _painter(painter)
{
}

QFontMetricsF TextHelper::fontMetrics() {
    return QFontMetricsF(_painter.font());
}

QPointF TextHelper::toTopCenter(const QString &text) {
    QFontMetricsF fontMetrics = this->fontMetrics();
    double hr = fontMetrics.horizontalAdvance(text);
    double asc = fontMetrics.ascent();
    return QPointF(hr / 2.0, -asc);
}

QPointF TextHelper::toBottomCenter(const QString &text) {
    QFontMetricsF fontMetrics = this->fontMetrics();
    double hr = fontMetrics.horizontalAdvance(text);
    double desc = fontMetrics.descent();
    return QPointF(hr / 2.0, desc);
}

void TextHelper::drawTextCenteredUp(QPointF point, const QString &text) {
    QPointF drawAt = point - toBottomCenter(text);
    _painter.drawText(drawAt, text);
}

void TextHelper::drawTextCenteredDown(QPointF point, const QString &text) {
    QPointF drawAt = point - toTopCenter(text);
    _painter.drawText(drawAt, text);
}

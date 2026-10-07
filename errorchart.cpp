#include "errorchart.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>

#include <algorithm>
#include <cmath>

ErrorChart::ErrorChart(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(172);
    setAccessibleName(QStringLiteral("График ошибки положения во времени"));
    setToolTip(QStringLiteral("Ошибка положения в метрах. Последние 60 секунд модели. "
                               "Пунктир отмечает ручную коррекцию; спад происходит мгновенно."));
    clear();
}

void ErrorChart::clear()
{
    samples.clear();
    corrections.clear();
    samples.append(QPointF(0, 0));
    update();
}

void ErrorChart::addSample(double time, double error)
{
    const QPointF sample(time, error);
    if (samples.isEmpty() || samples.last() != sample)
        samples.append(sample);
    trimHistory(time);
    update();
}

void ErrorChart::markCorrection(double time)
{
    if (corrections.isEmpty() || corrections.last() != time)
        corrections.append(time);
    trimHistory(time);
    update();
}

void ErrorChart::trimHistory(double time)
{
    const double left = std::max(0.0, time - historySeconds);
    int removeCount = 0;
    // Один образец слева от окна нужен, чтобы кривая доходила до границы.
    while (removeCount + 1 < samples.size() && samples[removeCount + 1].x() < left)
        ++removeCount;
    if (removeCount > 0)
        samples.remove(0, removeCount);
    while (!corrections.isEmpty() && corrections.first() < left)
        corrections.removeFirst();
}

void ErrorChart::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#25344b"), 1));
    painter.setBrush(QColor("#101b2d"));
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 14, 14);

    QFont labelFont = font();
    labelFont.setPixelSize(12);
    painter.setFont(labelFont);
    painter.setPen(QColor("#dce8fa"));
    painter.drawText(QPointF(18, 26), QStringLiteral("ОШИБКА ПОЛОЖЕНИЯ"));
    painter.setPen(QColor("#91a5c3"));
    painter.drawText(QRectF(width() - 280, 12, 260, 20),
                     Qt::AlignRight | Qt::AlignVCenter,
                     QStringLiteral("e(t), м  ·  окно 60 с  ·  ↧ коррекция"));

    const QRectF plot(62, 44, width() - 82, height() - 76);
    if (plot.width() <= 0 || plot.height() <= 0 || samples.isEmpty())
        return;

    const double time = samples.last().x();
    const double right = std::max(10.0, time);
    const double left = std::max(0.0, right - historySeconds);
    double largest = 0.0;
    for (const QPointF &sample : samples)
        largest = std::max(largest, sample.y());

    // Шкала по Y всегда подписана. Нижняя граница — ровно ноль, без обрезания.
    const double requestedTop = std::max(0.01, largest * 1.12);
    const double unit = std::pow(10.0, std::floor(std::log10(requestedTop)));
    const double top = std::ceil(requestedTop / unit * 2.0) * unit / 2.0;
    const auto point = [&](const QPointF &sample) {
        return QPointF(plot.left() + (sample.x() - left) / (right - left) * plot.width(),
                       plot.bottom() - sample.y() / top * plot.height());
    };

    for (int i = 0; i <= 2; ++i) {
        const double fraction = i / 2.0;
        const qreal y = plot.bottom() - fraction * plot.height();
        painter.setPen(QPen(QColor("#26364d"), 1));
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        painter.setPen(QColor("#91a5c3"));
        painter.drawText(QRectF(6, y - 10, 47, 20), Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(top * fraction, 'g', 3));
    }
    for (int i = 0; i <= 4; ++i) {
        const double fraction = i / 4.0;
        const qreal x = plot.left() + fraction * plot.width();
        painter.setPen(QPen(QColor(38, 54, 77, 130), 1));
        painter.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
        painter.setPen(QColor("#91a5c3"));
        const QRectF label(x - 25, plot.bottom() + 6, 50, 18);
        painter.drawText(label, Qt::AlignCenter,
                         QStringLiteral("%1 с").arg(left + fraction * (right - left), 0, 'f', 1));
    }

    painter.save();
    painter.setClipRect(plot.adjusted(-3, -5, 3, 3));
    for (double correctedAt : corrections) {
        const qreal x = point(QPointF(correctedAt, 0)).x();
        painter.setPen(QPen(QColor(81, 224, 170, 110), 1, Qt::DashLine));
        painter.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#51e0aa"));
        QPolygonF triangle;
        triangle << QPointF(x - 4, plot.top() - 3) << QPointF(x + 4, plot.top() - 3)
                 << QPointF(x, plot.top() + 3);
        painter.drawPolygon(triangle);
    }

    QPainterPath line;
    line.moveTo(point(samples.first()));
    for (int i = 1; i < samples.size(); ++i)
        line.lineTo(point(samples[i]));
    QPainterPath area = line;
    area.lineTo(QPointF(point(samples.last()).x(), plot.bottom()));
    area.lineTo(QPointF(point(samples.first()).x(), plot.bottom()));
    area.closeSubpath();
    QLinearGradient fill(plot.topLeft(), plot.bottomLeft());
    fill.setColorAt(0, QColor(244, 200, 106, 48));
    fill.setColorAt(1, QColor(244, 200, 106, 2));
    painter.fillPath(area, fill);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor("#f4c86a"), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(line);
    painter.setBrush(QColor("#f4c86a"));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(point(samples.last()), 3, 3);
    painter.restore();
}

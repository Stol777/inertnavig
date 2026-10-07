#include "navigationview.h"

#include <QGraphicsScene>
#include <QLinearGradient>
#include <QPainter>
#include <QRadialGradient>
#include <QResizeEvent>

#include <algorithm>
#include <cmath>

NavigationView::NavigationView(QWidget *parent) : QGraphicsView(parent)
{
    setFrameShape(QFrame::NoFrame);
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFocusPolicy(Qt::NoFocus);
    // Полная перерисовка нужна и для подписей поверх перемещающихся объектов.
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
}

void NavigationView::fitWorld()
{
    if (!scene() || scene()->sceneRect().isEmpty())
        return;

    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    const QRectF bounds = scene()->sceneRect();
    const double desired = std::max(bounds.width(), bounds.height()) / 12.0;
    const double unit = std::pow(10.0, std::floor(std::log10(desired)));
    const double fraction = desired / unit;
    const double multiplier = fraction <= 1.0 ? 1.0 : fraction <= 2.0 ? 2.0
                                                   : fraction <= 5.0 ? 5.0 : 10.0;
    gridStep = multiplier * unit;
    viewport()->update();
}

void NavigationView::setRunning(bool active)
{
    running = active;
    viewport()->update();
}

void NavigationView::setPositions(const QPointF &truth, const QPointF &estimate,
                                  double error)
{
    truePosition = truth;
    estimatedPosition = estimate;
    positionError = error;
    viewport()->update();
}

void NavigationView::drawBackground(QPainter *painter, const QRectF &rect)
{
    // Градиент закреплён за экраном, а физическая сетка — за координатами сцены.
    painter->save();
    painter->resetTransform();
    QLinearGradient background(0, 0, viewport()->width(), viewport()->height());
    background.setColorAt(0.0, QColor("#15243b"));
    background.setColorAt(1.0, QColor("#0a1220"));
    painter->fillRect(viewport()->rect(), background);
    QRadialGradient light(QPointF(viewport()->width() * 0.58,
                                  viewport()->height() * 0.35),
                          viewport()->width() * 0.65);
    light.setColorAt(0.0, QColor(73, 138, 235, 23));
    light.setColorAt(1.0, QColor(73, 138, 235, 0));
    painter->fillRect(viewport()->rect(), light);
    painter->restore();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    const auto grid = [&](double spacing, const QColor &color) {
        QPen pen(color, 0);
        painter->setPen(pen);
        for (double x = std::ceil(rect.left() / spacing) * spacing;
             x <= rect.right(); x += spacing)
            painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
        for (double y = std::ceil(rect.top() / spacing) * spacing;
             y <= rect.bottom(); y += spacing)
            painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    };

    if (gridStep * transform().m11() / 5.0 >= 12.0)
        grid(gridStep / 5.0, QColor(145, 174, 213, 10));
    grid(gridStep, QColor(145, 174, 213, 28));
    painter->setPen(QPen(QColor(145, 174, 213, 70), 0));
    painter->drawLine(QPointF(rect.left(), 0), QPointF(rect.right(), 0));
    painter->drawLine(QPointF(0, rect.top()), QPointF(0, rect.bottom()));
    painter->restore();
}

void NavigationView::paintEvent(QPaintEvent *event)
{
    QGraphicsView::paintEvent(event);
    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing);
    const int w = viewport()->width();
    const int h = viewport()->height();
    QFont small = font();
    small.setPixelSize(12);
    painter.setFont(small);

    // Подпись измерения остаётся читаемой при любом масштабе сцены.
    const QPointF first = mapFromScene(truePosition);
    const QPointF second = mapFromScene(estimatedPosition);
    if (QLineF(first, second).length() >= 54.0 && positionError > 0.0) {
        const QString text = QStringLiteral("Δr = %1 м").arg(positionError, 0, 'f', 3);
        const qreal labelWidth = painter.fontMetrics().horizontalAdvance(text) + 20;
        const QPointF middle = (first + second) / 2.0;
        const qreal x = std::max(12.0, std::min(middle.x() - labelWidth / 2.0,
                                              w - labelWidth - 12.0));
        const qreal y = std::max(76.0, std::min(middle.y() - 35.0, h - 64.0));
        const QRectF label(x, y, labelWidth, 26);
        painter.setPen(QPen(QColor(244, 200, 106, 90), 1));
        painter.setBrush(QColor(17, 27, 44, 240));
        painter.drawRoundedRect(label, 7, 7);
        painter.setPen(QColor("#f4c86a"));
        painter.drawText(label, Qt::AlignCenter, text);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(11, 20, 35, 220));
    painter.drawRoundedRect(QRectF(12, 12, 252, 56), 10, 10);
    QFont title = small;
    title.setWeight(QFont::DemiBold);
    title.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
    painter.setFont(title);
    painter.setPen(QColor("#dce8fa"));
    painter.drawText(QPointF(24, 33), QStringLiteral("ТРАЕКТОРИИ"));
    painter.setFont(small);

    const auto legendEntry = [&](int x, const QColor &color, const QString &text) {
        painter.setPen(QPen(color, 2.5, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(x, 51), QPointF(x + 16, 51));
        painter.setPen(QColor("#b0bfd5"));
        painter.drawText(QPointF(x + 24, 55), text);
    };
    legendEntry(24, QColor("#51e0aa"), QStringLiteral("Истина"));
    legendEntry(135, QColor("#66a9ff"), QStringLiteral("Оценка"));

    const QRectF badge(w - 147, 16, 131, 26);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(244, 200, 106, 18));
    painter.drawRoundedRect(badge, 13, 13);
    painter.setPen(QColor("#f4c86a"));
    painter.drawText(badge, Qt::AlignCenter, QStringLiteral("МОДЕЛЬ · GPS OFF"));

    const qreal barLength = gridStep * transform().m11();
    painter.setPen(QPen(QColor("#9badc6"), 1.2));
    painter.drawLine(QPointF(24, h - 24), QPointF(24 + barLength, h - 24));
    painter.drawLine(QPointF(24, h - 28), QPointF(24, h - 20));
    painter.drawLine(QPointF(24 + barLength, h - 28), QPointF(24 + barLength, h - 20));
    painter.drawText(QPointF(24, h - 35), QStringLiteral("%1 м · основная сетка")
                                                .arg(gridStep, 0, 'g', 3));
    painter.drawText(QRectF(w - 150, h - 43, 126, 26), Qt::AlignRight | Qt::AlignVCenter,
                     QStringLiteral("X →     Y ↑"));

    if (!running) {
        painter.setPen(QColor("#91a5c3"));
        painter.drawText(QRectF(20, h * 0.28, w - 40, 28), Qt::AlignCenter,
                         QStringLiteral("Задай ускорение и нажми «Старт»"));
    }
}

void NavigationView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    fitWorld();
}

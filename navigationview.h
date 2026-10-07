#ifndef NAVIGATIONVIEW_H
#define NAVIGATIONVIEW_H

#include <QGraphicsView>
#include <QPointF>

// Сцена хранит траектории, этот вид рисует фон и подписи в экранных пикселях.
class NavigationView : public QGraphicsView
{
public:
    explicit NavigationView(QWidget *parent = nullptr);
    void fitWorld();
    void setRunning(bool running);
    void setPositions(const QPointF &truth, const QPointF &estimate, double error);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QPointF truePosition;
    QPointF estimatedPosition;
    double positionError = 0.0;
    double gridStep = 5.0;
    bool running = false;
};

#endif // NAVIGATIONVIEW_H

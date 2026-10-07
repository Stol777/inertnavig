#ifndef ERRORCHART_H
#define ERRORCHART_H

#include <QPointF>
#include <QVector>
#include <QWidget>

// График рисуется средствами Widgets: отдельный модуль Qt Charts не требуется.
class ErrorChart : public QWidget
{
public:
    explicit ErrorChart(QWidget *parent = nullptr);
    void addSample(double time, double error);
    void markCorrection(double time);
    void clear();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void trimHistory(double time);
    QVector<QPointF> samples;
    QVector<double> corrections;
    static constexpr double historySeconds = 60.0;
};

#endif // ERRORCHART_H

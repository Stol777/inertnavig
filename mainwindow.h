#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "simulationmodel.h"

#include <QMainWindow>
#include <QPainterPath>
#include <QRectF>
#include <QString>

class QGraphicsEllipseItem;
class QGraphicsLineItem;
class QGraphicsPathItem;
class QGraphicsScene;
class QTimer;
class QVariantAnimation;
#ifdef INS_WITH_SOUND
class QSoundEffect;
#endif

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void startSimulation();
    void correctPosition();
    void resetSimulation();
    void tick();

private:
    void updateScene();
    void updateStatus();
    void setupSound();
    void updateSoundControl();

    Ui::MainWindow *ui;
    QGraphicsScene *scene;
    QTimer *timer;
    QVariantAnimation *correctionAnimation;
    SimulationModel model;

    QPainterPath truePath;
    QPainterPath estimatedPath;
    QGraphicsPathItem *trueItem;
    QGraphicsPathItem *estimatedItem;
    QGraphicsPathItem *trueGlow;
    QGraphicsPathItem *estimatedGlow;
    QGraphicsPathItem *trueMarker;
    QGraphicsPathItem *estimatedMarker;
    QGraphicsLineItem *errorLine;
    QGraphicsEllipseItem *correctionRing;

    QRectF sceneBounds;
    QString status;
    int correctionCount = 0;
    double lastCorrectionTime = 0.0;
    double lastCorrectionError = 0.0;
#ifdef INS_WITH_SOUND
    QSoundEffect *correctionSound = nullptr;
#endif
};

#endif // MAINWINDOW_H

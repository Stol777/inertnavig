#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QBrush>
#include <QColor>
#include <QEasingCurve>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QLineEdit>
#include <QPen>
#include <QPushButton>
#include <QStyle>
#include <QTimer>
#include <QVariantAnimation>
#ifdef INS_WITH_SOUND
#include <QSoundEffect>
#include <QUrl>
#endif

#include <algorithm>
#include <cmath>

namespace {

const QColor truthColor("#51e0aa");
const QColor estimateColor("#66a9ff");
const QColor errorColor("#f4c86a");

QPointF scenePoint(double x, double y)
{
    // В модели Y направлена вверх, в QGraphicsScene — вниз.
    return QPointF(x, -y);
}

bool readAcceleration(const QLineEdit *edit, double &value)
{
    QString text = edit->text().trimmed();
    text.replace(',', '.');
    bool ok = false;
    value = text.toDouble(&ok);
    return ok && std::isfinite(value) && value >= -10.0 && value <= 10.0;
}

void setInputError(QLineEdit *edit, bool invalid)
{
    edit->setProperty("invalid", invalid);
    edit->style()->unpolish(edit);
    edit->style()->polish(edit);
    edit->update();
}

QPen tracePen(const QColor &color, qreal width)
{
    QPen pen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    pen.setCosmetic(true);
    return pen;
}

void updateMarker(QGraphicsPathItem *marker, double vx, double vy, double radius)
{
    QPainterPath shape;
    if (std::hypot(vx, vy) < 1e-12) {
        shape.addEllipse(QPointF(0, 0), radius, radius);
        marker->setRotation(0);
    } else {
        shape.moveTo(11, 0);
        shape.lineTo(-7, -6);
        shape.lineTo(-3, 0);
        shape.lineTo(-7, 6);
        shape.closeSubpath();
        const double radiansToDegrees = 180.0 / std::acos(-1.0);
        marker->setRotation(-std::atan2(vy, vx) * radiansToDegrees);
    }
    marker->setPath(shape);
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      scene(new QGraphicsScene(this)),
      timer(new QTimer(this)),
      correctionAnimation(new QVariantAnimation(this))
{
    ui->setupUi(this);
    ui->graphicsView->setScene(scene);
    // Геометрия путей постоянно меняется; индекс сцены здесь не нужен.
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);

    QColor faintTruth = truthColor;
    QColor faintEstimate = estimateColor;
    faintTruth.setAlpha(30);
    faintEstimate.setAlpha(35);
    trueGlow = scene->addPath(truePath, tracePen(faintTruth, 9));
    estimatedGlow = scene->addPath(estimatedPath, tracePen(faintEstimate, 8));
    trueGlow->setZValue(0);
    estimatedGlow->setZValue(0);
    trueItem = scene->addPath(truePath, tracePen(truthColor, 3));
    estimatedItem = scene->addPath(estimatedPath, tracePen(estimateColor, 2));
    trueItem->setZValue(1);
    estimatedItem->setZValue(2);

    QPen errorPen(errorColor, 1.2, Qt::DashLine);
    errorPen.setCosmetic(true);
    errorLine = scene->addLine(QLineF(), errorPen);
    errorLine->setZValue(3);

    trueMarker = scene->addPath(QPainterPath(), tracePen(QColor("#d0ffec"), 1), truthColor);
    estimatedMarker = scene->addPath(QPainterPath(), tracePen(QColor("#d6e8ff"), 1), estimateColor);
    trueMarker->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    estimatedMarker->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    trueMarker->setZValue(5);
    estimatedMarker->setZValue(6);
    trueMarker->setToolTip(QStringLiteral("Истинная скорость: стрелка показывает направление движения"));
    estimatedMarker->setToolTip(QStringLiteral("Направление оценённой скорости"));

    correctionRing = scene->addEllipse(-10, -10, 20, 20, tracePen(truthColor, 2));
    correctionRing->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    correctionRing->setZValue(4);
    correctionRing->hide();
    correctionAnimation->setDuration(750);
    correctionAnimation->setStartValue(0.0);
    correctionAnimation->setEndValue(1.0);
    correctionAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(correctionAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant &value) {
        const double progress = value.toDouble();
        const double radius = 10.0 + 44.0 * progress;
        correctionRing->setRect(-radius, -radius, 2.0 * radius, 2.0 * radius);
        correctionRing->setOpacity(1.0 - progress);
    });
    connect(correctionAnimation, &QVariantAnimation::finished,
            this, [this] { correctionRing->hide(); });

    timer->setInterval(50);
    timer->setTimerType(Qt::PreciseTimer);
    connect(timer, &QTimer::timeout, this, &MainWindow::tick);
    connect(ui->btnStart, &QPushButton::clicked, this, &MainWindow::startSimulation);
    connect(ui->btnCorrect, &QPushButton::clicked, this, &MainWindow::correctPosition);
    connect(ui->btnReset, &QPushButton::clicked, this, &MainWindow::resetSimulation);
    connect(ui->lineEditAx, &QLineEdit::returnPressed, this, &MainWindow::startSimulation);
    connect(ui->lineEditAy, &QLineEdit::returnPressed, this, &MainWindow::startSimulation);
    for (QLineEdit *edit : {ui->lineEditAx, ui->lineEditAy})
        connect(edit, &QLineEdit::textEdited, this, [edit] { setInputError(edit, false); });

    setupSound();
    resetSimulation();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::startSimulation()
{
    if (timer->isActive())
        return;
    double ax = 0.0;
    double ay = 0.0;
    const bool validX = readAcceleration(ui->lineEditAx, ax);
    const bool validY = readAcceleration(ui->lineEditAy, ay);
    setInputError(ui->lineEditAx, !validX);
    setInputError(ui->lineEditAy, !validY);
    if (!validX || !validY) {
        status = QStringLiteral("Введите ускорения от −10 до 10 м/с²");
        updateStatus();
        QLineEdit *invalidEdit = validX ? ui->lineEditAy : ui->lineEditAx;
        invalidEdit->setFocus();
        invalidEdit->selectAll();
        return;
    }

    model.ax = ax;
    model.ay = ay;
    ui->lineEditAx->setEnabled(false);
    ui->lineEditAy->setEnabled(false);
    ui->btnStart->setEnabled(false);
    ui->btnCorrect->setEnabled(true);
    ui->graphicsView->setRunning(true);
    status = QStringLiteral("Движение · внешняя позиция недоступна");
    timer->start();
    updateStatus();
}

void MainWindow::correctPosition()
{
    if (!timer->isActive())
        return;

    lastCorrectionTime = model.time();
    lastCorrectionError = model.error();
    // Две точки с одинаковым временем дают вертикальный спад на графике.
    ui->errorChart->addSample(model.time(), model.error());
    model.correctPosition();
    ui->errorChart->addSample(model.time(), model.error());
    ui->errorChart->markCorrection(model.time());
    ++correctionCount;

    // Позиция меняется сразу. Анимация не вмешивается в состояние модели.
    estimatedPath.moveTo(scenePoint(model.xEst, model.yEst));
    correctionAnimation->stop();
    correctionRing->setPos(scenePoint(model.xTrue, model.yTrue));
    correctionRing->setRect(-10, -10, 20, 20);
    correctionRing->setOpacity(1.0);
    correctionRing->show();
    correctionAnimation->start();
#ifdef INS_WITH_SOUND
    if (ui->btnSound->isChecked() && correctionSound->status() == QSoundEffect::Ready) {
        correctionSound->stop();
        correctionSound->play();
    }
#endif
    status = QStringLiteral("Позиция уточнена · движение продолжается");
    updateScene();
    updateStatus();
}

void MainWindow::resetSimulation()
{
    timer->stop();
    correctionAnimation->stop();
    correctionRing->hide();
#ifdef INS_WITH_SOUND
    correctionSound->stop();
#endif
    model.reset();
    correctionCount = 0;
    lastCorrectionTime = lastCorrectionError = 0.0;
    ui->errorChart->clear();
    truePath = QPainterPath();
    estimatedPath = QPainterPath();
    truePath.moveTo(0.0, 0.0);
    estimatedPath.moveTo(0.0, 0.0);

    sceneBounds = QRectF(-20.0, -15.0, 40.0, 30.0);
    scene->setSceneRect(sceneBounds);
    ui->graphicsView->fitWorld();
    ui->graphicsView->setRunning(false);
    ui->lineEditAx->setEnabled(true);
    ui->lineEditAy->setEnabled(true);
    setInputError(ui->lineEditAx, false);
    setInputError(ui->lineEditAy, false);
    ui->btnStart->setEnabled(true);
    ui->btnCorrect->setEnabled(false);
    status = QStringLiteral("Готово к запуску");
    updateScene();
    updateStatus();
}

void MainWindow::tick()
{
    model.step();
    truePath.lineTo(scenePoint(model.xTrue, model.yTrue));
    estimatedPath.lineTo(scenePoint(model.xEst, model.yEst));
    ui->errorChart->addSample(model.time(), model.error());
    updateScene();
    updateStatus();
}

void MainWindow::updateScene()
{
    trueItem->setPath(truePath);
    estimatedItem->setPath(estimatedPath);
    trueGlow->setPath(truePath);
    estimatedGlow->setPath(estimatedPath);
    const QPointF truth = scenePoint(model.xTrue, model.yTrue);
    const QPointF estimate = scenePoint(model.xEst, model.yEst);
    trueMarker->setPos(truth);
    estimatedMarker->setPos(estimate);
    updateMarker(trueMarker, model.vxTrue, model.vyTrue, 6);
    updateMarker(estimatedMarker, model.vxEst, model.vyEst, 3.5);
    errorLine->setLine(QLineF(truth, estimate));
    errorLine->setVisible(model.error() > 0.0);

    // Область только расширяется: вся пространственная история остаётся видна.
    QRectF bounds = sceneBounds;
    for (const QPointF &point : {truth, estimate}) {
        const double inset = 0.16 * std::min(bounds.width(), bounds.height());
        if (!bounds.adjusted(inset, inset, -inset, -inset).contains(point)) {
            const double margin = 0.24 * std::max(bounds.width(), bounds.height());
            bounds.setLeft(std::min(bounds.left(), point.x() - margin));
            bounds.setRight(std::max(bounds.right(), point.x() + margin));
            bounds.setTop(std::min(bounds.top(), point.y() - margin));
            bounds.setBottom(std::max(bounds.bottom(), point.y() + margin));
        }
    }
    if (bounds != sceneBounds) {
        sceneBounds = bounds;
        scene->setSceneRect(sceneBounds);
        ui->graphicsView->fitWorld();
    }
    ui->graphicsView->setPositions(truth, estimate, model.error());
}

void MainWindow::updateStatus()
{
    const QString stateColor = timer->isActive() ? QStringLiteral("#51e0aa")
                                                : QStringLiteral("#91a5c3");
    const QString last = correctionCount == 0
        ? QStringLiteral("Коррекций пока не было")
        : QStringLiteral("Последняя: %1 с<br/>Ошибка до коррекции: %2 м")
              .arg(lastCorrectionTime, 0, 'f', 2).arg(lastCorrectionError, 0, 'f', 3);

    ui->labelStatus->setText(
        QStringLiteral("<p style='color:%1; margin:0 0 10px 0;'>%2</p>"
                       "<p style='margin:0; color:#f4c86a;'><span style='font-size:34px;'>%3</span> м</p>"
                       "<p style='color:#91a5c3; margin:2px 0 18px 0;'>ошибка положения</p>"
                       "<table width='100%' cellspacing='0' cellpadding='4'>"
                       "<tr><td>Время модели</td><td align='right'>%4 с</td></tr>"
                       "<tr><td>Δx</td><td align='right'>%5 м</td></tr>"
                       "<tr><td>Δy</td><td align='right'>%6 м</td></tr>"
                       "<tr><td>Коррекций</td><td align='right'>%7</td></tr></table>"
                       "<p style='color:#91a5c3; margin:16px 0 0 0;'>%8</p>")
            .arg(stateColor, status.toHtmlEscaped())
            .arg(model.error(), 0, 'f', 3)
            .arg(model.time(), 0, 'f', 2)
            .arg(model.xEst - model.xTrue, 0, 'f', 3)
            .arg(model.yEst - model.yTrue, 0, 'f', 3)
            .arg(correctionCount)
            .arg(last));
}

void MainWindow::setupSound()
{
#ifdef INS_WITH_SOUND
    correctionSound = new QSoundEffect(this);
    correctionSound->setVolume(0.22f);
    correctionSound->setLoopCount(1);
    connect(correctionSound, &QSoundEffect::statusChanged,
            this, &MainWindow::updateSoundControl);
    connect(ui->btnSound, &QPushButton::toggled, this, [this](bool enabled) {
        correctionSound->setMuted(!enabled);
        if (!enabled)
            correctionSound->stop();
        updateSoundControl();
    });
    correctionSound->setSource(QUrl(QStringLiteral("qrc:/audio/correction.wav")));
#endif
    updateSoundControl();
}

void MainWindow::updateSoundControl()
{
#ifdef INS_WITH_SOUND
    const bool ready = correctionSound->status() == QSoundEffect::Ready;
    const bool failed = correctionSound->status() == QSoundEffect::Error;
    ui->btnSound->setEnabled(ready);
    ui->btnSound->setText(ready
        ? (ui->btnSound->isChecked() ? QStringLiteral("Звук коррекции: вкл")
                                     : QStringLiteral("Звук коррекции: выкл"))
        : (failed ? QStringLiteral("Звук недоступен") : QStringLiteral("Подготовка звука…")));
    ui->btnSound->setToolTip(failed
        ? QStringLiteral("Звуковой файл или аудиоустройство недоступны. Визуальные эффекты работают.")
        : QStringLiteral("Короткий тихий сигнал только при ручной коррекции. Нажми, чтобы переключить."));
#else
    ui->btnSound->setEnabled(false);
    ui->btnSound->setChecked(false);
    ui->btnSound->setText(QStringLiteral("Звук недоступен"));
    ui->btnSound->setToolTip(QStringLiteral("Эта сборка создана без Qt Multimedia. "
                                            "График и анимация доступны."));
#endif
}

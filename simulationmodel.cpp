#include "simulationmodel.h"

#include <cmath>

void SimulationModel::step()
{
    // При постоянном ускорении это точное решение за один шаг.
    // Сначала обновляем координаты со старой скоростью, затем саму скорость.
    xTrue += vxTrue * dt + 0.5 * ax * dt * dt;
    yTrue += vyTrue * dt + 0.5 * ay * dt * dt;
    vxTrue += ax * dt;
    vyTrue += ay * dt;

    const double measuredAx = ax + bias;
    const double measuredAy = ay + bias;

    xEst += vxEst * dt + 0.5 * measuredAx * dt * dt;
    yEst += vyEst * dt + 0.5 * measuredAy * dt * dt;
    vxEst += measuredAx * dt;
    vyEst += measuredAy * dt;

    ++stepCount;
}

void SimulationModel::correctPosition()
{
    xEst = xTrue;
    yEst = yTrue;
    // Внешняя коррекция сообщает только позицию. Ошибка скорости остаётся.
}

void SimulationModel::reset()
{
    xTrue = yTrue = vxTrue = vyTrue = 0.0;
    xEst = yEst = vxEst = vyEst = 0.0;
    stepCount = 0;
}

double SimulationModel::time() const
{
    return static_cast<double>(stepCount) * dt;
}

double SimulationModel::error() const
{
    return std::hypot(xEst - xTrue, yEst - yTrue);
}

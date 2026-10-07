#ifndef SIMULATIONMODEL_H
#define SIMULATIONMODEL_H

#include <cstdint>

class SimulationModel
{
public:
    static constexpr double dt = 0.05;   // шаг модельного времени, с
    static constexpr double bias = 0.02; // смещение акселерометра по каждой оси, м/с²

    double ax = 0.5;
    double ay = 0.3;

    double xTrue = 0.0;
    double yTrue = 0.0;
    double vxTrue = 0.0;
    double vyTrue = 0.0;

    double xEst = 0.0;
    double yEst = 0.0;
    double vxEst = 0.0;
    double vyEst = 0.0;

    void step();
    void correctPosition();
    void reset();
    double time() const;
    double error() const;

private:
    std::uint64_t stepCount = 0;
};

#endif // SIMULATIONMODEL_H

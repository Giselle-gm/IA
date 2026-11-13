
#ifndef EVALUADOR_H
#define EVALUADOR_H

#include "instance.h"
#include "solucion.h"

#include <string>
#include <vector>

struct Evaluacion {
    bool factible = false;

    double objectiveValue = 0.0;
    double travelCost = 0.0;
    double fixedCost = 0.0;

    int vehiculosUsados = 0;

    std::vector<std::string> violations;
};

int travelTime(const Location& from, const Location& to);
int travelCost(const Location& from, const Location& to);

Evaluacion evaluarSolucion(
    const Instance& instance,
    const Solucion& solucion
);

#endif

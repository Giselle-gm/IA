#ifndef SIMULATED_ANNEALING_H
#define SIMULATED_ANNEALING_H

#include "instance.h"
#include "solucion.h"
#include "evaluador.h"

struct SAConfig {
    int iterations = 5000;
    double initialTemperature = 100.0;
    double coolingRate = 0.995;
    double minimumTemperature = 0.01;
    unsigned int randomSeed = 42;
};

struct SAResult {
    bool success = false;
    Solucion bestSolucion;
    Evaluacion initialEvaluacion;
    Evaluacion bestEvaluacion;
    int iterationsExecuted = 0;
    int acceptedMoves = 0;
};

// Mejora una solución inicial factible mediante Simulated Annealing.
SAResult simulatedAnnealing(
    const Instance& instance,
    const Solucion& initialSolucion,
    const SAConfig& config = SAConfig{}
);

#endif

#ifndef SIMULATED_ANNEALING_H
#define SIMULATED_ANNEALING_H

#include "instance.h"
#include "solution.h"
#include "evaluator.h"

struct SAConfig {
    int iterations = 5000;
    double initialTemperature = 100.0;
    double coolingRate = 0.995;
    double minimumTemperature = 0.01;
};

struct SAResult {
    bool success = false;
    Solution bestSolution;
    Evaluation initialEvaluation;
    Evaluation bestEvaluation;
    int iterationsExecuted = 0;
    int acceptedMoves = 0;
};

// Mejora una solución inicial factible mediante Simulated Annealing.
SAResult simulatedAnnealing(
    const Instance& instance,
    const Solution& initialSolution,
    const SAConfig& config = SAConfig{}
);

#endif

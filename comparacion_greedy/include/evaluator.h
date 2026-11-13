
#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "instance.h"
#include "solution.h"

#include <string>
#include <vector>

struct Evaluation {
    bool feasible = false;

    double objectiveValue = 0.0;
    double travelCost = 0.0;
    double fixedCost = 0.0;

    int vehiclesUsed = 0;

    std::vector<std::string> violations;
};

int travelTime(const Location& from, const Location& to);
int travelCost(const Location& from, const Location& to);

Evaluation evaluateSolution(
    const Instance& instance,
    const Solution& solution
);

#endif

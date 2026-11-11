#ifndef GREEDY_H
#define GREEDY_H

#include "instance.h"
#include "solution.h"

#include <string>

enum class GreedyPriorityStrategy {
    OPTIONS_THEN_DEMAND,
    WINDOW_THEN_OPTIONS_THEN_DEMAND,
    OPTIONS_THEN_WINDOW_THEN_DEMAND,
    WINDOW_THEN_DEMAND_THEN_OPTIONS
};

struct GreedyConfig {
    int maxSearchNodes = 20000;
    int maxBacktracks = 5000;

    // Por defecto conserva exactamente el criterio Greedy original.
    GreedyPriorityStrategy priorityStrategy =
        GreedyPriorityStrategy::OPTIONS_THEN_DEMAND;
};

struct GreedyResult {
    bool success = false;
    Solution solution;
    std::string message;
};

// Construye una solución inicial mediante inserciones Greedy.
GreedyResult buildGreedySolution(
    const Instance& instance,
    const GreedyConfig& config = GreedyConfig{}
);

#endif

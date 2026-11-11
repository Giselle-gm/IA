#ifndef GREEDY_H
#define GREEDY_H

#include "instance.h"
#include "solution.h"

#include <string>

struct GreedyConfig {
    int maxSearchNodes = 20000;
    int maxBacktracks = 5000;
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

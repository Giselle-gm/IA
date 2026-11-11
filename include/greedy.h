
#ifndef GREEDY_H
#define GREEDY_H

#include "instance.h"
#include "solution.h"

#include <string>

struct GreedyResult {
    bool success = false;
    Solution solution;
    std::string message;
};

// Construye una solución inicial mediante inserciones Greedy.
GreedyResult buildGreedySolution(const Instance& instance);

#endif

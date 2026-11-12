#ifndef GREEDY_H
#define GREEDY_H

#include "instance.h"
#include "solution.h"

#include <string>

struct GreedyConfig {
    int maxRestarts = 100;
    int restrictedCandidateListSize = 5;
    unsigned int randomSeed = 42;
};

struct GreedyResult {
    bool success = false;
    Solution solution;
    std::string message;
    int restartsAttempted = 0;
    int assignedCustomers = 0;
};

GreedyResult buildGreedySolution(
    const Instance& instance,
    const GreedyConfig& config = GreedyConfig{}
);

#endif

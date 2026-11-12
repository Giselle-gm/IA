#ifndef GREEDY_H
#define GREEDY_H

#include "instance.h"
#include "solution.h"

#include <string>
#include <vector>

enum class GreedyStrategy {
    RANDOM_ORDER,
    FEWEST_OPTIONS_FIRST,
    MOST_CONSTRAINED_FIRST
};

struct GreedyConfig {
    int maxRestarts = 100;
    int restrictedCandidateListSize = 5;
    unsigned int randomSeed = 42;
    GreedyStrategy strategy = GreedyStrategy::FEWEST_OPTIONS_FIRST;
};

struct RestartStats {
    int attempt = 0;
    std::vector<int> customerOrder;
    std::vector<int> selectedOptionOrder;
    int assignedCustomers = 0;
    bool feasible = false;
    std::string failureReason;
    double elapsedSeconds = 0.0;
};

struct GreedyResult {
    bool success = false;
    Solution solution;
    std::string message;
    int restartsAttempted = 0;
    int assignedCustomers = 0;
    std::vector<RestartStats> restartStats;
};

GreedyResult buildGreedySolution(
    const Instance& instance,
    const GreedyConfig& config = GreedyConfig{}
);

std::string greedyStrategyName(GreedyStrategy strategy);

#endif

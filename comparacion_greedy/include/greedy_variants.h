#ifndef GREEDY_VARIANTS_H
#define GREEDY_VARIANTS_H
#include "instance.h"
#include "solution.h"
#include <string>
struct GreedyVariantResult {
    bool success = false;
    Solution solution;
    std::string message;
    int exploredNodes = 0;
    int backtracks = 0;
    int assignedCustomers = 0;
};
GreedyVariantResult buildGreedyBacktracking(const Instance& instance);
GreedyVariantResult buildGreedyNoBacktracking(const Instance& instance);
#endif

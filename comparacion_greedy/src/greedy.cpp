#include "greedy.h"
#include "evaluator.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <random>
#include <set>
#include <string>
#include <tuple>
#include <vector>

using namespace std;
using Clock = chrono::steady_clock;

namespace {

constexpr double FIXED_VEHICLE_COST = 100000.0;
constexpr double PRIORITY_PENALTY = 100.0;

struct Candidate {
    int customerId;
    int optionId;
    int routeIndex;
    int position;
    double score;
};

double routeTravelCost(const Instance& instance, const vector<int>& route) {
    if (route.empty()) return 0.0;

    double cost = 0.0;
    int previousLocationId = 0;
    bool hasPreviousStop = false;

    for (int optionId : route) {
        const int locationId = instance.options[optionId].locationId;
        if (hasPreviousStop && locationId == previousLocationId) continue;

        cost += travelCost(
            instance.locations[previousLocationId],
            instance.locations[locationId]
        );
        previousLocationId = locationId;
        hasPreviousStop = true;
    }

    cost += travelCost(
        instance.locations[previousLocationId],
        instance.locations[0]
    );
    return cost;
}

bool routeIsFeasible(const Instance& instance, const vector<int>& route) {
    int load = 0;
    int currentTime = 0;
    int previousLocationId = 0;
    int serviceDuration = 0;
    bool hasCurrentStop = false;

    for (int optionId : route) {
        if (optionId < 0 || optionId >= static_cast<int>(instance.options.size()))
            return false;

        const Option& option = instance.options[optionId];
        if (option.customerId < 0 ||
            option.customerId >= static_cast<int>(instance.customers.size()) ||
            option.locationId <= 0 ||
            option.locationId >= static_cast<int>(instance.locations.size()))
            return false;

        load += instance.customers[option.customerId].demand;
        if (load > instance.vehicleCapacity) return false;

        const Location& location = instance.locations[option.locationId];

        if (!hasCurrentStop || option.locationId != previousLocationId) {
            if (hasCurrentStop) currentTime += serviceDuration;

            currentTime += travelTime(
                instance.locations[previousLocationId], location
            );

            const int serviceStart = max(currentTime, location.ready);
            if (serviceStart > location.due) return false;

            currentTime = serviceStart;
            serviceDuration = location.serviceTime;
            previousLocationId = option.locationId;
            hasCurrentStop = true;
        }

        serviceDuration += option.serviceTime;
    }
    return true;
}

bool serviceLevelsCanStillBeMet(
    const Instance& instance,
    const vector<int>& selectedOptions,
    int candidateCustomer,
    int candidateOption
) {
    const int customerCount = static_cast<int>(instance.customers.size());

    for (int priority = 0;
         priority < static_cast<int>(instance.minimumServiceLevel.size());
         ++priority) {
        int satisfied = 0;
        int potential = 0;

        for (int customerId = 0; customerId < customerCount; ++customerId) {
            int selectedOption = selectedOptions[customerId];
            if (customerId == candidateCustomer) selectedOption = candidateOption;

            if (selectedOption >= 0) {
                if (instance.options[selectedOption].priority <= priority)
                    ++satisfied;
                continue;
            }

            bool hasSuitableOption = false;
            for (int optionId : instance.optionsByCustomer[customerId]) {
                if (optionId < 0 ||
                    optionId >= static_cast<int>(instance.options.size()))
                    continue;
                if (instance.options[optionId].priority <= priority) {
                    hasSuitableOption = true;
                    break;
                }
            }
            if (hasSuitableOption) ++potential;
        }

        const int required = static_cast<int>(
            ceil(instance.minimumServiceLevel[priority] * customerCount)
        );
        if (satisfied + potential < required) return false;
    }
    return true;
}

bool insertionIsFeasible(
    const Instance& instance,
    const vector<int>& route,
    int optionId,
    int position,
    const vector<int>& sdlUsage
) {
    if (position < 0 || position > static_cast<int>(route.size()) ||
        optionId < 0 || optionId >= static_cast<int>(instance.options.size()))
        return false;

    const Option& option = instance.options[optionId];
    if (option.customerId < 0 ||
        option.customerId >= static_cast<int>(instance.customers.size()) ||
        option.locationId <= 0 ||
        option.locationId >= static_cast<int>(instance.locations.size()))
        return false;

    const Location& location = instance.locations[option.locationId];
    if (location.type == 1 && location.capacity >= 0 &&
        sdlUsage[option.locationId] + 1 > location.capacity)
        return false;

    vector<int> candidateRoute = route;
    candidateRoute.insert(candidateRoute.begin() + position, optionId);
    return routeIsFeasible(instance, candidateRoute);
}

double insertionScore(
    const Instance& instance,
    const vector<int>& route,
    int optionId,
    int position,
    bool opensNewRoute
) {
    vector<int> candidateRoute = route;
    candidateRoute.insert(candidateRoute.begin() + position, optionId);

    const double travelDelta =
        routeTravelCost(instance, candidateRoute) -
        routeTravelCost(instance, route);
    const double fixedCost = opensNewRoute ? FIXED_VEHICLE_COST : 0.0;

    return fixedCost + travelDelta +
           instance.options[optionId].priority * PRIORITY_PENALTY;
}

vector<Candidate> generateCandidates(
    const Instance& instance,
    const vector<vector<int>>& routes,
    const vector<int>& selectedOptions,
    const vector<int>& sdlUsage
) {
    vector<Candidate> candidates;

    for (int customerId = 0;
         customerId < static_cast<int>(instance.customers.size());
         ++customerId) {
        if (selectedOptions[customerId] >= 0) continue;

        for (int optionId : instance.optionsByCustomer[customerId]) {
            if (optionId < 0 ||
                optionId >= static_cast<int>(instance.options.size()))
                continue;

            const Option& option = instance.options[optionId];
            if (option.customerId != customerId ||
                !serviceLevelsCanStillBeMet(
                    instance, selectedOptions, customerId, optionId))
                continue;

            for (int routeIndex = 0;
                 routeIndex < static_cast<int>(routes.size());
                 ++routeIndex) {
                const vector<int>& route = routes[routeIndex];

                for (int position = 0;
                     position <= static_cast<int>(route.size());
                     ++position) {
                    if (!insertionIsFeasible(
                            instance, route, optionId, position, sdlUsage))
                        continue;

                    candidates.push_back({
                        customerId, optionId, routeIndex, position,
                        insertionScore(instance, route, optionId, position, false)
                    });
                }
            }

            if (static_cast<int>(routes.size()) < instance.vehicleCount) {
                const vector<int> emptyRoute;
                if (insertionIsFeasible(
                        instance, emptyRoute, optionId, 0, sdlUsage)) {
                    candidates.push_back({
                        customerId, optionId, static_cast<int>(routes.size()), 0,
                        insertionScore(instance, emptyRoute, optionId, 0, true)
                    });
                }
            }
        }
    }

    sort(candidates.begin(), candidates.end(),
        [](const Candidate& a, const Candidate& b) {
            if (a.score != b.score) return a.score < b.score;
            if (a.customerId != b.customerId) return a.customerId < b.customerId;
            if (a.optionId != b.optionId) return a.optionId < b.optionId;
            if (a.routeIndex != b.routeIndex) return a.routeIndex < b.routeIndex;
            return a.position < b.position;
        });
    return candidates;
}

int countFeasibleOptions(const vector<Candidate>& candidates, int customerId) {
    set<int> options;
    for (const Candidate& candidate : candidates)
        if (candidate.customerId == customerId)
            options.insert(candidate.optionId);
    return static_cast<int>(options.size());
}

int minimumWindowWidth(const Instance& instance, const vector<Candidate>& candidates,
                       int customerId) {
    int width = numeric_limits<int>::max();
    for (const Candidate& candidate : candidates) {
        if (candidate.customerId != customerId) continue;
        const Location& location =
            instance.locations[instance.options[candidate.optionId].locationId];
        width = min(width, location.due - location.ready);
    }
    return width;
}

int minimumRemainingSdlCapacity(
    const Instance& instance,
    const vector<Candidate>& candidates,
    const vector<int>& sdlUsage,
    int customerId
) {
    int remaining = numeric_limits<int>::max();
    for (const Candidate& candidate : candidates) {
        if (candidate.customerId != customerId) continue;
        const Location& location =
            instance.locations[instance.options[candidate.optionId].locationId];
        if (location.type == 1 && location.capacity >= 0) {
            remaining = min(
                remaining, location.capacity - sdlUsage[location.id]
            );
        }
    }
    return remaining;
}

int bestPriority(const Instance& instance, const vector<Candidate>& candidates,
                 int customerId) {
    int priority = numeric_limits<int>::max();
    for (const Candidate& candidate : candidates)
        if (candidate.customerId == customerId)
            priority = min(priority, instance.options[candidate.optionId].priority);
    return priority;
}

int chooseCustomer(
    const Instance& instance,
    const vector<Candidate>& candidates,
    const vector<int>& sdlUsage,
    GreedyStrategy strategy,
    mt19937& generator
) {
    vector<int> availableCustomers;
    for (const Candidate& candidate : candidates) {
        if (availableCustomers.empty() ||
            find(availableCustomers.begin(), availableCustomers.end(),
                 candidate.customerId) == availableCustomers.end())
            availableCustomers.push_back(candidate.customerId);
    }

    if (strategy == GreedyStrategy::RANDOM_ORDER) {
        uniform_int_distribution<int> distribution(
            0, static_cast<int>(availableCustomers.size()) - 1
        );
        return availableCustomers[distribution(generator)];
    }

    if (strategy == GreedyStrategy::FEWEST_OPTIONS_FIRST) {
        return *min_element(
            availableCustomers.begin(), availableCustomers.end(),
            [&](int a, int b) {
                const int countA = countFeasibleOptions(candidates, a);
                const int countB = countFeasibleOptions(candidates, b);
                if (countA != countB) return countA < countB;
                return a < b;
            }
        );
    }

    return *min_element(
        availableCustomers.begin(), availableCustomers.end(),
        [&](int a, int b) {
            const auto keyA = make_tuple(
                countFeasibleOptions(candidates, a),
                minimumWindowWidth(instance, candidates, a),
                -instance.customers[a].demand,
                minimumRemainingSdlCapacity(instance, candidates, sdlUsage, a),
                bestPriority(instance, candidates, a),
                a
            );
            const auto keyB = make_tuple(
                countFeasibleOptions(candidates, b),
                minimumWindowWidth(instance, candidates, b),
                -instance.customers[b].demand,
                minimumRemainingSdlCapacity(instance, candidates, sdlUsage, b),
                bestPriority(instance, candidates, b),
                b
            );
            return keyA < keyB;
        }
    );
}

Solution convertToSolution(
    const Instance& instance,
    const vector<vector<int>>& routes,
    const vector<int>& selectedOptions
) {
    Solution solution;
    solution.selectedOptions = selectedOptions;

    for (const vector<int>& routeOptions : routes) {
        if (routeOptions.empty()) continue;

        Route route;
        route.vehicleId = static_cast<int>(solution.routes.size());

        for (int optionId : routeOptions) {
            const Option& option = instance.options[optionId];
            if (!route.stops.empty() &&
                route.stops.back().locationId == option.locationId) {
                route.stops.back().optionIds.push_back(optionId);
            } else {
                Stop stop;
                stop.locationId = option.locationId;
                stop.optionIds.push_back(optionId);
                route.stops.push_back(stop);
            }
        }
        solution.routes.push_back(route);
    }
    return solution;
}

} // namespace

string greedyStrategyName(GreedyStrategy strategy) {
    switch (strategy) {
        case GreedyStrategy::RANDOM_ORDER: return "Aleatorio";
        case GreedyStrategy::FEWEST_OPTIONS_FIRST: return "Menos opciones primero";
        case GreedyStrategy::MOST_CONSTRAINED_FIRST: return "Mayor dificultad primero";
    }
    return "Desconocida";
}

GreedyResult buildGreedySolution(
    const Instance& instance,
    const GreedyConfig& config
) {
    GreedyResult result;
    const int customerCount = static_cast<int>(instance.customers.size());
    const int locationCount = static_cast<int>(instance.locations.size());

    if (config.maxRestarts <= 0 || config.restrictedCandidateListSize <= 0) {
        result.message = "Los limites de reinicio y de candidatos deben ser positivos.";
        return result;
    }

    if (customerCount == 0 || locationCount == 0 ||
        instance.vehicleCount <= 0 || instance.vehicleCapacity <= 0 ||
        static_cast<int>(instance.optionsByCustomer.size()) != customerCount) {
        result.message = "La instancia no contiene datos suficientes para construir una solucion.";
        return result;
    }

    mt19937 generator(config.randomSeed);
    int bestPartialAssignment = 0;

    for (int restart = 1; restart <= config.maxRestarts; ++restart) {
        const auto restartStart = Clock::now();
        RestartStats stats;
        stats.attempt = restart;

        vector<vector<int>> routes;
        vector<int> selectedOptions(customerCount, -1);
        vector<int> sdlUsage(locationCount, 0);
        int assignedCustomers = 0;
        bool stuck = false;

        while (assignedCustomers < customerCount) {
            vector<Candidate> candidates = generateCandidates(
                instance, routes, selectedOptions, sdlUsage
            );

            if (candidates.empty()) {
                stuck = true;
                stats.failureReason =
                    "Sin inserciones factibles restantes: revisar capacidad vehicular, "
                    "ventanas temporales, capacidad SDL y niveles de servicio.";
                break;
            }

            const int customerId = chooseCustomer(
                instance, candidates, sdlUsage, config.strategy, generator
            );

            vector<Candidate> customerCandidates;
            for (const Candidate& candidate : candidates)
                if (candidate.customerId == customerId)
                    customerCandidates.push_back(candidate);

            const int rclSize = min(
                config.restrictedCandidateListSize,
                static_cast<int>(customerCandidates.size())
            );
            uniform_int_distribution<int> chooseCandidate(0, rclSize - 1);
            const Candidate& chosen =
                customerCandidates[chooseCandidate(generator)];

            if (chosen.routeIndex == static_cast<int>(routes.size()))
                routes.push_back({});

            vector<int>& selectedRoute = routes[chosen.routeIndex];
            selectedRoute.insert(
                selectedRoute.begin() + chosen.position, chosen.optionId
            );
            selectedOptions[chosen.customerId] = chosen.optionId;

            const Location& selectedLocation =
                instance.locations[instance.options[chosen.optionId].locationId];
            if (selectedLocation.type == 1)
                ++sdlUsage[selectedLocation.id];

            stats.customerOrder.push_back(chosen.customerId);
            stats.selectedOptionOrder.push_back(chosen.optionId);
            ++assignedCustomers;
        }

        stats.assignedCustomers = assignedCustomers;
        bestPartialAssignment = max(bestPartialAssignment, assignedCustomers);

        if (!stuck) {
            Solution solution = convertToSolution(
                instance, routes, selectedOptions
            );
            const Evaluation evaluation = evaluateSolution(instance, solution);

            if (!evaluation.feasible) {
                stats.failureReason = "La evaluacion final rechazo la solucion: ";
                if (!evaluation.violations.empty())
                    stats.failureReason += evaluation.violations.front();
                else
                    stats.failureReason += "violacion no especificada.";
            } else {
                solution.objectiveValue = evaluation.objectiveValue;
                stats.feasible = true;
                stats.elapsedSeconds = chrono::duration<double>(
                    Clock::now() - restartStart
                ).count();
                result.restartStats.push_back(stats);
                result.success = true;
                result.solution = solution;
                result.assignedCustomers = customerCount;
                result.restartsAttempted = restart;
                result.message =
                    "Solucion inicial factible construida. Intentos realizados: " +
                    to_string(restart) + ". Clientes asignados: " +
                    to_string(customerCount) + ".";
                return result;
            }
        }

        if (stats.failureReason.empty())
            stats.failureReason = "El intento no produjo una solucion factible.";

        stats.elapsedSeconds = chrono::duration<double>(
            Clock::now() - restartStart
        ).count();
        result.restartStats.push_back(stats);
    }

    result.restartsAttempted = config.maxRestarts;
    result.assignedCustomers = bestPartialAssignment;
    result.message =
        "No se encontro una solucion inicial factible tras " +
        to_string(config.maxRestarts) +
        " reinicios. Mayor cantidad de clientes asignados en un intento: " +
        to_string(bestPartialAssignment) + "/" +
        to_string(customerCount) + ".";
    return result;
}

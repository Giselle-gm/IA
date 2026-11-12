#include "greedy.h"
#include "evaluator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <string>
#include <utility>
#include <vector>

using namespace std;

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

double routeTravelCost(
    const Instance& instance,
    const vector<int>& route
) {
    if (route.empty()) {
        return 0.0;
    }

    double cost = 0.0;
    int previousLocationId = 0;
    bool hasPreviousStop = false;

    for (int optionId : route) {
        const int locationId = instance.options[optionId].locationId;

        if (hasPreviousStop && locationId == previousLocationId) {
            continue;
        }

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

bool routeIsFeasible(
    const Instance& instance,
    const vector<int>& route
) {
    int load = 0;
    int currentTime = 0;
    int previousLocationId = 0;
    int serviceDuration = 0;
    bool hasCurrentStop = false;

    for (int optionId : route) {
        if (optionId < 0 ||
            optionId >= static_cast<int>(instance.options.size())) {
            return false;
        }

        const Option& option = instance.options[optionId];

        if (option.customerId < 0 ||
            option.customerId >= static_cast<int>(instance.customers.size()) ||
            option.locationId <= 0 ||
            option.locationId >= static_cast<int>(instance.locations.size())) {
            return false;
        }

        load += instance.customers[option.customerId].demand;
        if (load > instance.vehicleCapacity) {
            return false;
        }

        const Location& location = instance.locations[option.locationId];

        if (!hasCurrentStop || option.locationId != previousLocationId) {
            if (hasCurrentStop) {
                currentTime += serviceDuration;
            }

            currentTime += travelTime(
                instance.locations[previousLocationId],
                location
            );

            const int serviceStart = max(currentTime, location.ready);
            if (serviceStart > location.due) {
                return false;
            }

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
    const int customerCount =
        static_cast<int>(instance.customers.size());

    for (int priority = 0;
         priority < static_cast<int>(instance.minimumServiceLevel.size());
         ++priority) {

        int satisfied = 0;
        int potential = 0;

        for (int customerId = 0; customerId < customerCount; ++customerId) {
            int selectedOption = selectedOptions[customerId];

            if (customerId == candidateCustomer) {
                selectedOption = candidateOption;
            }

            if (selectedOption >= 0) {
                if (instance.options[selectedOption].priority <= priority) {
                    ++satisfied;
                }
                continue;
            }

            bool hasSuitableOption = false;

            for (int optionId : instance.optionsByCustomer[customerId]) {
                if (optionId < 0 ||
                    optionId >= static_cast<int>(instance.options.size())) {
                    continue;
                }

                if (instance.options[optionId].priority <= priority) {
                    hasSuitableOption = true;
                    break;
                }
            }

            if (hasSuitableOption) {
                ++potential;
            }
        }

        const int required = static_cast<int>(
            ceil(instance.minimumServiceLevel[priority] * customerCount)
        );

        if (satisfied + potential < required) {
            return false;
        }
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
        optionId < 0 ||
        optionId >= static_cast<int>(instance.options.size())) {
        return false;
    }

    const Option& option = instance.options[optionId];

    if (option.customerId < 0 ||
        option.customerId >= static_cast<int>(instance.customers.size()) ||
        option.locationId <= 0 ||
        option.locationId >= static_cast<int>(instance.locations.size())) {
        return false;
    }

    const Location& location = instance.locations[option.locationId];

    if (location.type == 1 &&
        location.capacity >= 0 &&
        sdlUsage[option.locationId] + 1 > location.capacity) {
        return false;
    }

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

    const double fixedCost =
        opensNewRoute ? FIXED_VEHICLE_COST : 0.0;

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

        if (selectedOptions[customerId] >= 0) {
            continue;
        }

        for (int optionId : instance.optionsByCustomer[customerId]) {
            if (optionId < 0 ||
                optionId >= static_cast<int>(instance.options.size())) {
                continue;
            }

            const Option& option = instance.options[optionId];

            if (option.customerId != customerId ||
                !serviceLevelsCanStillBeMet(
                    instance, selectedOptions, customerId, optionId)) {
                continue;
            }

            for (int routeIndex = 0;
                 routeIndex < static_cast<int>(routes.size());
                 ++routeIndex) {

                const vector<int>& route = routes[routeIndex];

                for (int position = 0;
                     position <= static_cast<int>(route.size());
                     ++position) {

                    if (!insertionIsFeasible(
                            instance, route, optionId, position, sdlUsage)) {
                        continue;
                    }

                    candidates.push_back({
                        customerId,
                        optionId,
                        routeIndex,
                        position,
                        insertionScore(
                            instance, route, optionId, position, false
                        )
                    });
                }
            }

            if (static_cast<int>(routes.size()) < instance.vehicleCount) {
                const vector<int> emptyRoute;

                if (insertionIsFeasible(
                        instance, emptyRoute, optionId, 0, sdlUsage)) {
                    candidates.push_back({
                        customerId,
                        optionId,
                        static_cast<int>(routes.size()),
                        0,
                        insertionScore(
                            instance, emptyRoute, optionId, 0, true
                        )
                    });
                }
            }
        }
    }

    sort(
        candidates.begin(),
        candidates.end(),
        [](const Candidate& a, const Candidate& b) {
            if (a.score != b.score) {
                return a.score < b.score;
            }
            if (a.customerId != b.customerId) {
                return a.customerId < b.customerId;
            }
            if (a.optionId != b.optionId) {
                return a.optionId < b.optionId;
            }
            if (a.routeIndex != b.routeIndex) {
                return a.routeIndex < b.routeIndex;
            }
            return a.position < b.position;
        }
    );

    return candidates;
}

Solution convertToSolution(
    const Instance& instance,
    const vector<vector<int>>& routes,
    const vector<int>& selectedOptions
) {
    Solution solution;
    solution.selectedOptions = selectedOptions;

    for (int routeIndex = 0;
         routeIndex < static_cast<int>(routes.size());
         ++routeIndex) {

        if (routes[routeIndex].empty()) {
            continue;
        }

        Route route;
        route.vehicleId = static_cast<int>(solution.routes.size());

        for (int optionId : routes[routeIndex]) {
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

}

GreedyResult buildGreedySolution(
    const Instance& instance,
    const GreedyConfig& config
) {
    GreedyResult result;

    const int customerCount =
        static_cast<int>(instance.customers.size());
    const int locationCount =
        static_cast<int>(instance.locations.size());

    if (config.maxRestarts <= 0 ||
        config.restrictedCandidateListSize <= 0) {
        result.message =
            "Los limites de reinicio y de candidatos deben ser positivos.";
        return result;
    }

    if (customerCount == 0 ||
        locationCount == 0 ||
        instance.vehicleCount <= 0 ||
        instance.vehicleCapacity <= 0 ||
        static_cast<int>(instance.optionsByCustomer.size()) != customerCount) {
        result.message =
            "La instancia no contiene datos suficientes para construir una solucion.";
        return result;
    }

    mt19937 generator(config.randomSeed);
    int bestPartialAssignment = 0;

    for (int restart = 1; restart <= config.maxRestarts; ++restart) {
        result.restartsAttempted = restart;

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
                break;
            }

            const int rclSize = min(
                config.restrictedCandidateListSize,
                static_cast<int>(candidates.size())
            );

            uniform_int_distribution<int> chooseCandidate(0, rclSize - 1);
            const Candidate& chosen = candidates[chooseCandidate(generator)];

            if (chosen.routeIndex == static_cast<int>(routes.size())) {
                routes.push_back({});
            }

            vector<int>& selectedRoute = routes[chosen.routeIndex];
            selectedRoute.insert(
                selectedRoute.begin() + chosen.position,
                chosen.optionId
            );

            selectedOptions[chosen.customerId] = chosen.optionId;

            const Location& selectedLocation =
                instance.locations[instance.options[chosen.optionId].locationId];

            if (selectedLocation.type == 1) {
                ++sdlUsage[selectedLocation.id];
            }

            ++assignedCustomers;
        }

        bestPartialAssignment = max(bestPartialAssignment, assignedCustomers);

        if (stuck) {
            continue;
        }

        Solution solution = convertToSolution(
            instance, routes, selectedOptions
        );

        const Evaluation evaluation = evaluateSolution(instance, solution);

        if (!evaluation.feasible) {
            continue;
        }

        solution.objectiveValue = evaluation.objectiveValue;

        result.success = true;
        result.solution = solution;
        result.assignedCustomers = customerCount;
        result.message =
            "Solucion inicial factible construida con Greedy y reinicios. "
            "Intentos realizados: " + to_string(restart) +
            ". Clientes asignados: " + to_string(customerCount) + ".";

        return result;
    }

    result.assignedCustomers = bestPartialAssignment;
    result.message =
        "No se encontro una solucion inicial factible tras " +
        to_string(config.maxRestarts) +
        " reinicios. Mayor cantidad de clientes asignados en un intento: " +
        to_string(bestPartialAssignment) + "/" +
        to_string(customerCount) + ".";

    return result;
}

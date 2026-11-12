#include "greedy_sequential.h"

#include "evaluator.h"

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

using namespace std;

namespace {
    constexpr double FIXED_VEHICLE_COST = 100000.0;
    constexpr double PRIORITY_PENALTY = 100.0;

    bool isFeasibleInsertion(
        const Instance& instance,
        const vector<int>& route,
        int optionId,
        int position,
        const vector<int>& sdlUsage
    ) {
        const int customerCount = static_cast<int>(instance.customers.size());
        const int optionCount = static_cast<int>(instance.options.size());
        const int locationCount = static_cast<int>(instance.locations.size());

        if (optionId < 0 || optionId >= optionCount ||
            position < 0 || position > static_cast<int>(route.size())) {
            return false;
        }

        const Option& newOption = instance.options[optionId];
        if (newOption.customerId < 0 || newOption.customerId >= customerCount ||
            newOption.locationId < 0 || newOption.locationId >= locationCount) {
            return false;
        }

        const Location& newLocation = instance.locations[newOption.locationId];
        const int demand = instance.customers[newOption.customerId].demand;

        int totalLoad = demand;
        for (int routeOptionId : route) {
            if (routeOptionId < 0 || routeOptionId >= optionCount) {
                return false;
            }

            const int customerId = instance.options[routeOptionId].customerId;
            if (customerId < 0 || customerId >= customerCount) {
                return false;
            }
            totalLoad += instance.customers[customerId].demand;
        }

        if (totalLoad > instance.vehicleCapacity) {
            return false;
        }

        if (newLocation.type == 1 &&
            newLocation.capacity >= 0 &&
            (newLocation.id < 0 ||
             newLocation.id >= static_cast<int>(sdlUsage.size()) ||
             sdlUsage[newLocation.id] + 1 > newLocation.capacity)) {
            return false;
        }

        vector<int> candidateRoute = route;
        candidateRoute.insert(candidateRoute.begin() + position, optionId);

        int currentTime = 0;
        int previousLocationId = 0;
        int previousStopLocationId = -1;
        int serviceDurationAtStop = 0;

        for (size_t i = 0; i < candidateRoute.size(); ++i) {
            const Option& option = instance.options[candidateRoute[i]];
            const Location& location = instance.locations[option.locationId];

            currentTime += travelTime(
                instance.locations[previousLocationId],
                location
            );

            if (location.id != previousStopLocationId) {
                if (previousStopLocationId != -1) {
                    currentTime += serviceDurationAtStop;
                }

                const int serviceStart = max(currentTime, location.ready);
                if (serviceStart > location.due) {
                    return false;
                }

                currentTime = serviceStart;
                previousStopLocationId = location.id;
                serviceDurationAtStop = location.serviceTime;
            }

            serviceDurationAtStop += option.serviceTime;
            previousLocationId = location.id;
        }

        currentTime += serviceDurationAtStop;
        currentTime += travelTime(
            instance.locations[previousLocationId],
            instance.locations[0]
        );

        return true;
    }

    double insertionTravelDelta(
        const Instance& instance,
        const vector<int>& route,
        int optionId,
        int position
    ) {
        const int newLocationId = instance.options[optionId].locationId;
        const int previousLocationId =
            position == 0
                ? 0
                : instance.options[route[position - 1]].locationId;
        const int nextLocationId =
            position == static_cast<int>(route.size())
                ? 0
                : instance.options[route[position]].locationId;

        return travelCost(
                   instance.locations[previousLocationId],
                   instance.locations[newLocationId]
               ) +
               travelCost(
                   instance.locations[newLocationId],
                   instance.locations[nextLocationId]
               ) -
               travelCost(
                   instance.locations[previousLocationId],
                   instance.locations[nextLocationId]
               );
    }

    Route convertRoute(const vector<int>& optionIds, int vehicleId,
                       const Instance& instance) {
        Route route;
        route.vehicleId = vehicleId;

        for (int optionId : optionIds) {
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

        return route;
    }
}

GreedyResult buildGreedySequentialSolution(const Instance& instance) {
    GreedyResult result;

    const int customerCount = static_cast<int>(instance.customers.size());
    const int locationCount = static_cast<int>(instance.locations.size());
    const int optionCount = static_cast<int>(instance.options.size());

    if (customerCount == 0 || locationCount == 0 ||
        instance.vehicleCount <= 0 || instance.vehicleCapacity <= 0 ||
        static_cast<int>(instance.optionsByCustomer.size()) != customerCount) {
        result.message = "La instancia no contiene datos suficientes para construir una solucion.";
        return result;
    }

    vector<bool> customerAssigned(customerCount, false);
    vector<int> sdlUsage(locationCount, 0);
    vector<vector<int>> routes;

    Solution solution;
    solution.selectedOptions.assign(customerCount, -1);

    int assignedCount = 0;

    while (assignedCount < customerCount) {
        int bestCustomer = -1;
        int bestOption = -1;
        int bestRouteIndex = -1;
        int bestPosition = -1;
        double minimumCost = numeric_limits<double>::max();

        for (int customerId = 0; customerId < customerCount; ++customerId) {
            if (customerAssigned[customerId]) {
                continue;
            }

            for (int optionId : instance.optionsByCustomer[customerId]) {
                if (optionId < 0 || optionId >= optionCount) {
                    continue;
                }

                const Option& option = instance.options[optionId];
                if (option.customerId != customerId ||
                    option.locationId < 0 || option.locationId >= locationCount) {
                    continue;
                }

                for (size_t routeIndex = 0; routeIndex < routes.size(); ++routeIndex) {
                    const vector<int>& route = routes[routeIndex];

                    for (size_t position = 0; position <= route.size(); ++position) {
                        if (!isFeasibleInsertion(
                                instance, route, optionId,
                                static_cast<int>(position), sdlUsage)) {
                            continue;
                        }

                        const double cost =
                            insertionTravelDelta(
                                instance, route, optionId,
                                static_cast<int>(position)
                            ) +
                            option.priority * PRIORITY_PENALTY;

                        if (cost < minimumCost) {
                            minimumCost = cost;
                            bestCustomer = customerId;
                            bestOption = optionId;
                            bestRouteIndex = static_cast<int>(routeIndex);
                            bestPosition = static_cast<int>(position);
                        }
                    }
                }

                if (routes.size() < static_cast<size_t>(instance.vehicleCount)) {
                    const vector<int> emptyRoute;

                    if (isFeasibleInsertion(
                            instance, emptyRoute, optionId, 0, sdlUsage)) {
                        const Location& depot = instance.locations[0];
                        const Location& destination =
                            instance.locations[option.locationId];

                        const double deltaDistance =
                            travelCost(depot, destination) +
                            travelCost(destination, depot);

                        const double cost =
                            FIXED_VEHICLE_COST +
                            deltaDistance +
                            option.priority * PRIORITY_PENALTY;

                        if (cost < minimumCost) {
                            minimumCost = cost;
                            bestCustomer = customerId;
                            bestOption = optionId;
                            bestRouteIndex = static_cast<int>(routes.size());
                            bestPosition = 0;
                        }
                    }
                }
            }
        }

        if (bestCustomer == -1) {
            break;
        }

        if (bestRouteIndex == static_cast<int>(routes.size())) {
            routes.push_back({});
        }

        vector<int>& selectedRoute = routes[bestRouteIndex];
        selectedRoute.insert(
            selectedRoute.begin() + bestPosition,
            bestOption
        );

        solution.selectedOptions[bestCustomer] = bestOption;
        customerAssigned[bestCustomer] = true;

        const int locationId = instance.options[bestOption].locationId;
        if (instance.locations[locationId].type == 1) {
            sdlUsage[locationId]++;
        }

        assignedCount++;
    }

    for (size_t routeIndex = 0; routeIndex < routes.size(); ++routeIndex) {
        if (!routes[routeIndex].empty()) {
            solution.routes.push_back(
                convertRoute(
                    routes[routeIndex],
                    static_cast<int>(routeIndex),
                    instance
                )
            );
        }
    }

    const Evaluation evaluation = evaluateSolution(instance, solution);
    if (assignedCount != customerCount || !evaluation.feasible) {
        result.message =
            "Greedy por insercion factible no encontro una solucion completa. "
            "Clientes asignados: " + to_string(assignedCount) +
            "/" + to_string(customerCount) + ".";

        for (const string& violation : evaluation.violations) {
            result.message += "\n- " + violation;
        }

        result.solution = solution;
        return result;
    }

    solution.objectiveValue = evaluation.objectiveValue;
    result.success = true;
    result.solution = solution;
    result.message =
        "Solucion factible construida con Greedy por insercion factible.";
    return result;
}

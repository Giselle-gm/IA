#include "greedy_sequential.h"

#include "evaluator.h"

#include <cmath>
#include <limits>
#include <string>
#include <vector>

using namespace std;

GreedyResult buildGreedySequentialSolution(const Instance& instance) {
    GreedyResult result;

    const int customerCount = static_cast<int>(instance.customers.size());
    const int optionCount = static_cast<int>(instance.options.size());
    const int locationCount = static_cast<int>(instance.locations.size());

    if (customerCount == 0 || locationCount == 0 ||
        instance.vehicleCount <= 0 || instance.vehicleCapacity <= 0 ||
        static_cast<int>(instance.optionsByCustomer.size()) != customerCount) {
        result.message = "La instancia no contiene datos suficientes para construir una solucion.";
        return result;
    }

    Solution solution;
    solution.selectedOptions.assign(customerCount, -1);

    // Seleccionar una opción por cliente según prioridad y distancia al depósito.
    for (int customerId = 0; customerId < customerCount; customerId++) {
        double bestScore = numeric_limits<double>::max();
        int bestOptionId = -1;

        for (int optionId : instance.optionsByCustomer[customerId]) {
            if (optionId < 0 || optionId >= optionCount) {
                continue;
            }

            const Option& option = instance.options[optionId];
            if (option.customerId != customerId ||
                option.locationId < 0 || option.locationId >= locationCount) {
                continue;
            }

            const Location& depot = instance.locations[0];
            const Location& optionLocation = instance.locations[option.locationId];
            const double dx = depot.x - optionLocation.x;
            const double dy = depot.y - optionLocation.y;
            const double depotDistance = sqrt(dx * dx + dy * dy);
            const double score = option.priority * 500.0 + depotDistance;

            if (score < bestScore) {
                bestScore = score;
                bestOptionId = optionId;
            }
        }

        if (bestOptionId < 0) {
            result.message = "El cliente " + to_string(customerId) +
                             " no tiene una opcion valida.";
            return result;
        }

        solution.selectedOptions[customerId] = bestOptionId;
    }

    // Distribuir clientes secuencialmente, sin reordenar ni retroceder.
    int currentVehicle = 0;
    int currentLoad = 0;
    solution.routes.push_back(Route{});
    solution.routes.back().vehicleId = currentVehicle;

    for (int customerId = 0; customerId < customerCount; customerId++) {
        const int optionId = solution.selectedOptions[customerId];
        const Option& option = instance.options[optionId];
        const int demand = instance.customers[customerId].demand;

        if (currentLoad + demand > instance.vehicleCapacity &&
            currentVehicle + 1 < instance.vehicleCount) {
            currentVehicle++;
            currentLoad = 0;

            Route route;
            route.vehicleId = currentVehicle;
            solution.routes.push_back(route);
        }

        Stop stop;
        stop.locationId = option.locationId;
        stop.optionIds.push_back(optionId);
        solution.routes.back().stops.push_back(stop);

        currentLoad += demand;
    }

    // Unificar entregas consecutivas en una misma ubicación para contar
    // el tiempo de servicio de la ubicación una sola vez.
    for (Route& route : solution.routes) {
        vector<Stop> mergedStops;

        for (const Stop& stop : route.stops) {
            if (!mergedStops.empty() &&
                mergedStops.back().locationId == stop.locationId) {
                vector<int>& optionIds = mergedStops.back().optionIds;
                optionIds.insert(optionIds.end(),
                                 stop.optionIds.begin(),
                                 stop.optionIds.end());
            } else {
                mergedStops.push_back(stop);
            }
        }

        route.stops = mergedStops;
    }

    const Evaluation evaluation = evaluateSolution(instance, solution);
    if (!evaluation.feasible) {
        result.message = "El Greedy secuencial construyo una asignacion, pero no es factible.";
        for (const string& violation : evaluation.violations) {
            result.message += "\n- " + violation;
        }
        return result;
    }

    solution.objectiveValue = evaluation.objectiveValue;
    result.success = true;
    result.solution = solution;
    result.message = "Solucion factible construida con Greedy secuencial.";
    return result;
}

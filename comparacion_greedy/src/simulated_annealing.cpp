#include "simulated_annealing.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

using namespace std;

namespace {

    // Agrupa en una sola parada las visitas consecutivas a la misma ubicación.
    // Así, LOCATION.SERVICETIME se contabiliza una sola vez por visita física.
    void mergeConsecutiveStops(Solution& solution) {
        for (Route& route : solution.routes) {
            vector<Stop> mergedStops;

            for (const Stop& stop : route.stops) {
                if (!mergedStops.empty() &&
                    mergedStops.back().locationId == stop.locationId) {
                    vector<int>& options = mergedStops.back().optionIds;
                    options.insert(
                        options.end(),
                        stop.optionIds.begin(),
                        stop.optionIds.end()
                    );
                } else {
                    mergedStops.push_back(stop);
                }
            }

            route.stops = std::move(mergedStops);
        }
    }

    // Reasigna identificadores consecutivos después de modificar rutas.
    void normalizeVehicleIds(Solution& solution) {
        for (int i = 0; i < static_cast<int>(solution.routes.size()); ++i) {
            solution.routes[i].vehicleId = i;
        }
    }

    bool relocateStop(Solution& solution, mt19937& generator) {
        vector<pair<int, int>> positions;

        for (int r = 0; r < static_cast<int>(solution.routes.size()); ++r) {
            for (int s = 0; s < static_cast<int>(solution.routes[r].stops.size()); ++s) {
                positions.push_back({r, s});
            }
        }

        if (positions.empty()) return false;

        uniform_int_distribution<int> pickSource(0, static_cast<int>(positions.size()) - 1);
        const auto [sourceRoute, sourceStop] = positions[pickSource(generator)];

        // No dejar rutas vacías: el movimiento entre rutas requiere
        // que la ruta de origen tenga al menos dos paradas.
        if (solution.routes[sourceRoute].stops.size() == 1) return false;

        uniform_int_distribution<int> pickDestination(
            0, static_cast<int>(solution.routes.size()) - 1
        );
        const int destinationRoute = pickDestination(generator);

        Stop movedStop = solution.routes[sourceRoute].stops[sourceStop];
        solution.routes[sourceRoute].stops.erase(
            solution.routes[sourceRoute].stops.begin() + sourceStop
        );

        const int destinationSize =
            static_cast<int>(solution.routes[destinationRoute].stops.size());

        uniform_int_distribution<int> pickPosition(0, destinationSize);
        const int destinationPosition = pickPosition(generator);

        solution.routes[destinationRoute].stops.insert(
            solution.routes[destinationRoute].stops.begin() + destinationPosition,
            movedStop
        );

        return true;
    }

    bool swapStops(Solution& solution, mt19937& generator) {
        vector<pair<int, int>> positions;

        for (int r = 0; r < static_cast<int>(solution.routes.size()); ++r) {
            for (int s = 0; s < static_cast<int>(solution.routes[r].stops.size()); ++s) {
                positions.push_back({r, s});
            }
        }

        if (positions.size() < 2) return false;

        uniform_int_distribution<int> pick(0, static_cast<int>(positions.size()) - 1);
        const int first = pick(generator);
        int second = pick(generator);

        if (first == second) {
            second = (second + 1) % static_cast<int>(positions.size());
        }

        const auto [routeA, stopA] = positions[first];
        const auto [routeB, stopB] = positions[second];

        swap(
            solution.routes[routeA].stops[stopA],
            solution.routes[routeB].stops[stopB]
        );

        return true;
    }

    bool reverseRouteSegment(Solution& solution, mt19937& generator) {
        vector<int> eligibleRoutes;

        for (int r = 0; r < static_cast<int>(solution.routes.size()); ++r) {
            if (solution.routes[r].stops.size() >= 2) {
                eligibleRoutes.push_back(r);
            }
        }

        if (eligibleRoutes.empty()) return false;

        uniform_int_distribution<int> pickRoute(
            0, static_cast<int>(eligibleRoutes.size()) - 1
        );
        Route& route = solution.routes[eligibleRoutes[pickRoute(generator)]];

        uniform_int_distribution<int> pickFirst(
            0, static_cast<int>(route.stops.size()) - 2
        );
        const int first = pickFirst(generator);

        uniform_int_distribution<int> pickLast(
            first + 1, static_cast<int>(route.stops.size()) - 1
        );
        const int last = pickLast(generator);

        reverse(
            route.stops.begin() + first,
            route.stops.begin() + last + 1
        );

        return true;
    }

    bool generateNeighbor(Solution& candidate, mt19937& generator) {
        uniform_int_distribution<int> pickMove(0, 2);

        // Se intenta el operador elegido y, si no es aplicable,
        // se prueban los otros dos.
        const int firstMove = pickMove(generator);

        for (int offset = 0; offset < 3; ++offset) {
            const int move = (firstMove + offset) % 3;

            if (move == 0 && relocateStop(candidate, generator)) return true;
            if (move == 1 && swapStops(candidate, generator)) return true;
            if (move == 2 && reverseRouteSegment(candidate, generator)) return true;
        }

        return false;
    }
}

SAResult simulatedAnnealing(
    const Instance& instance,
    const Solution& initialSolution,
    const SAConfig& config
) {
    SAResult result;

    Solution currentSolution = initialSolution;
    mergeConsecutiveStops(currentSolution);

    result.initialEvaluation = evaluateSolution(instance, currentSolution);

    if (!result.initialEvaluation.feasible ||
        config.iterations <= 0 ||
        config.initialTemperature <= 0.0 ||
        config.coolingRate <= 0.0 ||
        config.coolingRate >= 1.0 ||
        config.minimumTemperature <= 0.0) {
        result.bestSolution = currentSolution;
        result.bestEvaluation = result.initialEvaluation;
        return result;
    }


    currentSolution.objectiveValue = result.initialEvaluation.objectiveValue;

    result.bestSolution = currentSolution;
    result.bestEvaluation = result.initialEvaluation;

    mt19937 generator(42);
    uniform_real_distribution<double> probability(0.0, 1.0);

    double temperature = config.initialTemperature;

    for (int iteration = 0;
         iteration < config.iterations && temperature > config.minimumTemperature;
         ++iteration) {

        result.iterationsExecuted++;

        Solution candidate = currentSolution;

        if (!generateNeighbor(candidate, generator)) {
            temperature *= config.coolingRate;
            continue;
        }

        mergeConsecutiveStops(candidate);
        normalizeVehicleIds(candidate);

        Evaluation candidateEvaluation = evaluateSolution(instance, candidate);

        if (candidateEvaluation.feasible) {
            const double delta =
                candidateEvaluation.objectiveValue -
                currentSolution.objectiveValue;

            const bool accept =
                delta <= 0.0 ||
                probability(generator) < exp(-delta / temperature);

            if (accept) {
                candidate.objectiveValue = candidateEvaluation.objectiveValue;
                currentSolution = candidate;
                result.acceptedMoves++;

                if (candidateEvaluation.objectiveValue <
                    result.bestEvaluation.objectiveValue) {
                    result.bestSolution = candidate;
                    result.bestEvaluation = candidateEvaluation;
                }
            }
        }

        temperature *= config.coolingRate;
    }

    result.success = result.bestEvaluation.feasible;
    result.bestSolution.objectiveValue =
        result.bestEvaluation.objectiveValue;

    return result;
}

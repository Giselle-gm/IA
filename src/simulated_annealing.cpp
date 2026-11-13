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
    void mergeConsecutiveStops(Solucion& solucion) {
        for (Route& route : solucion.routes) {
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
    void normalizeVehicleIds(Solucion& solucion) {
        for (int i = 0; i < static_cast<int>(solucion.routes.size()); ++i) {
            solucion.routes[i].vehicleId = i;
        }
    }

    bool relocateStop(Solucion& solucion, mt19937& generator) {
        vector<pair<int, int>> positions;

        for (int r = 0; r < static_cast<int>(solucion.routes.size()); ++r) {
            for (int s = 0; s < static_cast<int>(solucion.routes[r].stops.size()); ++s) {
                positions.push_back({r, s});
            }
        }

        if (positions.empty()) return false;

        uniform_int_distribution<int> pickSource(0, static_cast<int>(positions.size()) - 1);
        const auto [sourceRoute, sourceStop] = positions[pickSource(generator)];

        // No dejar rutas vacías: el movimiento entre rutas requiere
        // que la ruta de origen tenga al menos dos paradas.
        if (solucion.routes[sourceRoute].stops.size() == 1) return false;

        uniform_int_distribution<int> pickDestination(
            0, static_cast<int>(solucion.routes.size()) - 1
        );
        const int destinationRoute = pickDestination(generator);

        Stop movedStop = solucion.routes[sourceRoute].stops[sourceStop];
        solucion.routes[sourceRoute].stops.erase(
            solucion.routes[sourceRoute].stops.begin() + sourceStop
        );

        const int destinationSize =
            static_cast<int>(solucion.routes[destinationRoute].stops.size());

        uniform_int_distribution<int> pickPosition(0, destinationSize);
        const int destinationPosition = pickPosition(generator);

        solucion.routes[destinationRoute].stops.insert(
            solucion.routes[destinationRoute].stops.begin() + destinationPosition,
            movedStop
        );

        return true;
    }

    bool swapStops(Solucion& solucion, mt19937& generator) {
        vector<pair<int, int>> positions;

        for (int r = 0; r < static_cast<int>(solucion.routes.size()); ++r) {
            for (int s = 0; s < static_cast<int>(solucion.routes[r].stops.size()); ++s) {
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
            solucion.routes[routeA].stops[stopA],
            solucion.routes[routeB].stops[stopB]
        );

        return true;
    }

    bool reverseRouteSegment(Solucion& solucion, mt19937& generator) {
        vector<int> eligibleRoutes;

        for (int r = 0; r < static_cast<int>(solucion.routes.size()); ++r) {
            if (solucion.routes[r].stops.size() >= 2) {
                eligibleRoutes.push_back(r);
            }
        }

        if (eligibleRoutes.empty()) return false;

        uniform_int_distribution<int> pickRoute(
            0, static_cast<int>(eligibleRoutes.size()) - 1
        );
        Route& route = solucion.routes[eligibleRoutes[pickRoute(generator)]];

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

    bool generateNeighbor(Solucion& candidate, mt19937& generator) {
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
    const Solucion& initialSolucion,
    const SAConfig& config
) {
    SAResult result;

    Solucion currentSolucion = initialSolucion;
    mergeConsecutiveStops(currentSolucion);

    result.initialEvaluacion = evaluarSolucion(instance, currentSolucion);

    if (!result.initialEvaluacion.factible ||
        config.iterations <= 0 ||
        config.initialTemperature <= 0.0 ||
        config.coolingRate <= 0.0 ||
        config.coolingRate >= 1.0 ||
        config.minimumTemperature <= 0.0) {
        result.bestSolucion = currentSolucion;
        result.bestEvaluacion = result.initialEvaluacion;
        return result;
    }


    currentSolucion.objectiveValue = result.initialEvaluacion.objectiveValue;

    result.bestSolucion = currentSolucion;
    result.bestEvaluacion = result.initialEvaluacion;

    mt19937 generator(config.randomSeed);
    uniform_real_distribution<double> probability(0.0, 1.0);

    double temperature = config.initialTemperature;

    for (int iteration = 0;
         iteration < config.iterations && temperature > config.minimumTemperature;
         ++iteration) {

        result.iterationsExecuted++;

        Solucion candidate = currentSolucion;

        if (!generateNeighbor(candidate, generator)) {
            temperature *= config.coolingRate;
            continue;
        }

        mergeConsecutiveStops(candidate);
        normalizeVehicleIds(candidate);

        Evaluacion candidateEvaluacion = evaluarSolucion(instance, candidate);

        if (candidateEvaluacion.factible) {
            const double delta =
                candidateEvaluacion.objectiveValue -
                currentSolucion.objectiveValue;

            const bool accept =
                delta <= 0.0 ||
                probability(generator) < exp(-delta / temperature);

            if (accept) {
                candidate.objectiveValue = candidateEvaluacion.objectiveValue;
                currentSolucion = candidate;
                result.acceptedMoves++;

                if (candidateEvaluacion.objectiveValue <
                    result.bestEvaluacion.objectiveValue) {
                    result.bestSolucion = candidate;
                    result.bestEvaluacion = candidateEvaluacion;
                }
            }
        }

        temperature *= config.coolingRate;
    }

    result.success = result.bestEvaluacion.factible;
    result.bestSolucion.objectiveValue =
        result.bestEvaluacion.objectiveValue;

    return result;
}

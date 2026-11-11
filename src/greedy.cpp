
#include "greedy.h"
#include "evaluator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

using namespace std;

namespace {

    constexpr double FIXED_VEHICLE_COST = 100000.0;

    // Une paradas consecutivas que corresponden a la misma ubicación.
    // Así, varias entregas en una misma ubicación se atienden juntas.
    void mergeConsecutiveStops(Route& route) {

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

        route.stops = mergedStops;
    }


    // Calcula el costo de viaje de una ruta completa,
    // considerando el viaje desde y hacia el depósito 0.
    double calculateRouteCost(
        const Instance& instance,
        const Route& route
    ) {

        if (route.stops.empty()) {
            return 0.0;
        }

        double cost = 0.0;
        int previousLocationId = 0;

        for (const Stop& stop : route.stops) {

            const Location& previous =
                instance.locations[previousLocationId];

            const Location& current =
                instance.locations[stop.locationId];

            cost += travelCost(previous, current);

            previousLocationId = stop.locationId;
        }

        cost += travelCost(
            instance.locations[previousLocationId],
            instance.locations[0]
        );

        return cost;
    }


    // Calcula la función objetivo de una solución parcial.
    double calculatePartialObjective(
        const Instance& instance,
        const Solution& solution
    ) {

        double objective =
            FIXED_VEHICLE_COST * solution.routes.size();

        for (const Route& route : solution.routes) {
            objective += calculateRouteCost(instance, route);
        }

        return objective;
    }


    // Comprueba las restricciones que deben cumplirse
    // durante la construcción de la solución.
    //
    // Los niveles mínimos de servicio se comprueban
    // al finalizar, cuando todos los clientes están asignados.
    bool isPartialSolutionFeasible(
        const Instance& instance,
        const Solution& solution
    ) {

        const int customerCount =
            static_cast<int>(instance.customers.size());

        const int locationCount =
            static_cast<int>(instance.locations.size());

        const int optionCount =
            static_cast<int>(instance.options.size());

        // Comprobar cantidad de vehículos.
        if (static_cast<int>(solution.routes.size()) >
            instance.vehicleCount) {
            return false;
        }

        // --------------------------------------------------
        // 1. CAPACIDAD DE LAS RUTAS Y VENTANAS TEMPORALES
        // --------------------------------------------------

        for (const Route& route : solution.routes) {

            if (route.stops.empty()) {
                return false;
            }

            int routeLoad = 0;
            int currentTime = 0;
            int previousLocationId = 0;

            vector<bool> customerVisited(customerCount, false);

            for (const Stop& stop : route.stops) {

                int locationId = stop.locationId;

                if (locationId <= 0 || locationId >= locationCount) {
                    return false;
                }

                if (stop.optionIds.empty()) {
                    return false;
                }

                const Location& location =
                    instance.locations[locationId];

                const Location& previousLocation =
                    instance.locations[previousLocationId];

                currentTime += travelTime(
                    previousLocation,
                    location
                );

                int serviceStart = max(
                    currentTime,
                    location.ready
                );

                int serviceDuration = location.serviceTime;

                for (int optionId : stop.optionIds) {

                    if (optionId < 0 || optionId >= optionCount) {
                        return false;
                    }

                    const Option& option =
                        instance.options[optionId];

                    int customerId = option.customerId;

                    if (customerId < 0 || customerId >= customerCount) {
                        return false;
                    }

                    // La opción debe pertenecer al cliente
                    // que la tiene seleccionada.
                    if (solution.selectedOptions[customerId] != optionId) {
                        return false;
                    }

                    // La opción debe corresponder a esta ubicación.
                    if (option.locationId != locationId) {
                        return false;
                    }

                    // Un cliente no puede aparecer dos veces en la ruta.
                    if (customerVisited[customerId]) {
                        return false;
                    }

                    customerVisited[customerId] = true;

                    routeLoad +=
                        instance.customers[customerId].demand;

                    serviceDuration += option.serviceTime;
                }

                int serviceEnd =
                    serviceStart + serviceDuration;

                // Criterio provisional:
                // el servicio completo debe terminar antes de DUE.
                if (serviceEnd > location.due) {
                    return false;
                }

                currentTime = serviceEnd;
                previousLocationId = locationId;
            }

            if (routeLoad > instance.vehicleCapacity) {
                return false;
            }
        }

        // --------------------------------------------------
        // 2. CAPACIDAD GLOBAL DE UBICACIONES COMPARTIDAS
        // --------------------------------------------------

        vector<int> locationLoads(locationCount, 0);

        for (int customerId = 0;
             customerId < customerCount;
             customerId++) {

            int optionId =
                solution.selectedOptions[customerId];

            if (optionId < 0) {
                continue;
            }

            if (optionId >= optionCount) {
                return false;
            }

            const Option& option =
                instance.options[optionId];

            if (option.locationId < 0 ||
                option.locationId >= locationCount) {
                return false;
            }

            const Location& location =
                instance.locations[option.locationId];

            if (location.type == 1) {

                locationLoads[option.id >= 0
                    ? option.locationId
                    : option.locationId] +=
                    instance.customers[customerId].demand;
            }
        }

        for (int locationId = 0;
             locationId < locationCount;
             locationId++) {

            const Location& location =
                instance.locations[locationId];

            if (location.type == 1 &&
                location.capacity >= 0 &&
                locationLoads[locationId] > location.capacity) {
                return false;
            }
        }

        return true;
    }

}


// --------------------------------------------------
// CONSTRUCCIÓN DE LA SOLUCIÓN GREEDY
// --------------------------------------------------

GreedyResult buildGreedySolution(const Instance& instance) {

    GreedyResult result;

    const int customerCount =
        static_cast<int>(instance.customers.size());

    const int optionCount =
        static_cast<int>(instance.options.size());

    // Comprobaciones básicas de la instancia.
    if (customerCount == 0 ||
        instance.locations.empty() ||
        instance.vehicleCount <= 0 ||
        instance.vehicleCapacity <= 0) {

        result.message =
            "La instancia no contiene datos suficientes para construir una solucion.";

        return result;
    }

    if (static_cast<int>(instance.optionsByCustomer.size()) !=
        customerCount) {

        result.message =
            "La tabla optionsByCustomer no tiene un elemento por cliente.";

        return result;
    }

    // --------------------------------------------------
    // 1. INICIALIZAR SOLUCIÓN
    // --------------------------------------------------

    Solution currentSolution;

    // -1 indica que el cliente todavía no tiene opción asignada.
    currentSolution.selectedOptions.assign(
        customerCount,
        -1
    );

    // --------------------------------------------------
    // 2. ORDENAR CLIENTES POR DIFICULTAD
    // --------------------------------------------------

    vector<int> customerOrder(customerCount);

    for (int i = 0; i < customerCount; i++) {
        customerOrder[i] = i;
    }

    sort(
        customerOrder.begin(),
        customerOrder.end(),
        [&](int a, int b) {

            int optionsA =
                static_cast<int>(instance.optionsByCustomer[a].size());

            int optionsB =
                static_cast<int>(instance.optionsByCustomer[b].size());

            // Primero, clientes con menos alternativas.
            if (optionsA != optionsB) {
                return optionsA < optionsB;
            }

            // En caso de empate, mayor demanda primero.
            return instance.customers[a].demand >
                   instance.customers[b].demand;
        }
    );

    // --------------------------------------------------
    // 3. INSERTAR CADA CLIENTE
    // --------------------------------------------------

    for (int customerId : customerOrder) {

        const vector<int>& availableOptions =
            instance.optionsByCustomer[customerId];

        if (availableOptions.empty()) {

            result.message =
                "El cliente " + to_string(customerId) +
                " no tiene opciones disponibles.";

            return result;
        }

        bool foundCandidate = false;

        Solution bestCandidate;

        double bestObjective =
            numeric_limits<double>::infinity();

        int bestPriority =
            numeric_limits<int>::max();

        // Probar todas las opciones del cliente.
        for (int optionId : availableOptions) {

            if (optionId < 0 || optionId >= optionCount) {
                continue;
            }

            const Option& option =
                instance.options[optionId];

            if (option.customerId != customerId) {
                continue;
            }

            // --------------------------------------------------
            // A. INSERTAR EN UNA RUTA EXISTENTE
            // --------------------------------------------------

            for (int routeIndex = 0;
                 routeIndex < static_cast<int>(currentSolution.routes.size());
                 routeIndex++) {

                const Route& originalRoute =
                    currentSolution.routes[routeIndex];

                // Probar todas las posiciones posibles.
                for (int position = 0;
                     position <= static_cast<int>(originalRoute.stops.size());
                     position++) {

                    Solution candidate = currentSolution;

                    candidate.selectedOptions[customerId] =
                        optionId;

                    Route& candidateRoute =
                        candidate.routes[routeIndex];

                    Stop newStop;
                    newStop.locationId = option.locationId;
                    newStop.optionIds.push_back(optionId);

                    candidateRoute.stops.insert(
                        candidateRoute.stops.begin() + position,
                        newStop
                    );

                    // Agrupar visitas consecutivas a la misma ubicación.
                    mergeConsecutiveStops(candidateRoute);

                    if (!isPartialSolutionFeasible(
                        instance,
                        candidate
                    )) {
                        continue;
                    }

                    double candidateObjective =
                        calculatePartialObjective(
                            instance,
                            candidate
                        );

                    // Elegir menor objetivo.
                    // En empate, favorecer menor prioridad numérica.
                    if (!foundCandidate ||
                        candidateObjective < bestObjective ||
                        (candidateObjective == bestObjective &&
                         option.priority < bestPriority)) {

                        foundCandidate = true;
                        bestObjective = candidateObjective;
                        bestPriority = option.priority;
                        bestCandidate = candidate;
                    }
                }
            }

            // --------------------------------------------------
            // B. CREAR UNA RUTA NUEVA
            // --------------------------------------------------

            if (static_cast<int>(currentSolution.routes.size()) <
                instance.vehicleCount) {

                Solution candidate = currentSolution;

                candidate.selectedOptions[customerId] =
                    optionId;

                Route newRoute;

                newRoute.vehicleId =
                    static_cast<int>(candidate.routes.size());

                Stop newStop;
                newStop.locationId = option.locationId;
                newStop.optionIds.push_back(optionId);

                newRoute.stops.push_back(newStop);

                candidate.routes.push_back(newRoute);

                if (isPartialSolutionFeasible(
                    instance,
                    candidate
                )) {

                    double candidateObjective =
                        calculatePartialObjective(
                            instance,
                            candidate
                        );

                    if (!foundCandidate ||
                        candidateObjective < bestObjective ||
                        (candidateObjective == bestObjective &&
                         option.priority < bestPriority)) {

                        foundCandidate = true;
                        bestObjective = candidateObjective;
                        bestPriority = option.priority;
                        bestCandidate = candidate;
                    }
                }
            }
        }

        // --------------------------------------------------
        // C. CONFIRMAR LA MEJOR INSERCIÓN
        // --------------------------------------------------

        if (!foundCandidate) {

            result.message =
                "No fue posible insertar al cliente " +
                to_string(customerId) +
                " sin incumplir las restricciones.";

            return result;
        }

        currentSolution = bestCandidate;
    }

    // --------------------------------------------------
    // 4. EVALUAR SOLUCIÓN COMPLETA
    // --------------------------------------------------

    Evaluation evaluation =
        evaluateSolution(instance, currentSolution);

    if (!evaluation.feasible) {

        result.message =
            "Greedy construyo una solucion, pero no cumple todas las restricciones.";

        for (const string& violation : evaluation.violations) {
            result.message += "\n- " + violation;
        }

        return result;
    }

    // Guardar el valor objetivo calculado por el evaluador.
    currentSolution.objectiveValue =
        evaluation.objectiveValue;

    result.success = true;
    result.solution = currentSolution;
    result.message =
        "Solucion inicial factible construida correctamente.";

    return result;
}

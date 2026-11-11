
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


    // Comprueba si los niveles mínimos de servicio todavía
    // pueden alcanzarse con los clientes que faltan por asignar.
    bool serviceLevelsCanStillBeMet(
        const Instance& instance,
        const Solution& solution
    ) {
        const int customerCount =
            static_cast<int>(instance.customers.size());

        for (int priority = 0;
             priority < static_cast<int>(
                 instance.minimumServiceLevel.size()
             );
             priority++) {

            int satisfiedCustomers = 0;
            int potentialCustomers = 0;

            for (int customerId = 0;
                 customerId < customerCount;
                 customerId++) {

                int optionId =
                    solution.selectedOptions[customerId];

                if (optionId >= 0) {
                    if (instance.options[optionId].priority <= priority) {
                        satisfiedCustomers++;
                    }
                    continue;
                }

                bool hasSuitableOption = false;

                for (int availableOptionId :
                     instance.optionsByCustomer[customerId]) {

                    if (availableOptionId < 0 ||
                        availableOptionId >=
                            static_cast<int>(instance.options.size())) {
                        continue;
                    }

                    if (instance.options[availableOptionId].priority
                        <= priority) {
                        hasSuitableOption = true;
                        break;
                    }
                }

                if (hasSuitableOption) {
                    potentialCustomers++;
                }
            }

            int requiredCustomers = static_cast<int>(
                ceil(
                    instance.minimumServiceLevel[priority]
                    * customerCount
                )
            );

            if (satisfiedCustomers + potentialCustomers
                < requiredCustomers) {
                return false;
            }
        }

        return true;
    }


    // Comprueba las restricciones que deben cumplirse
    // durante la construcción de la solución.
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

        if (static_cast<int>(solution.routes.size()) >
            instance.vehicleCount) {
            return false;
        }


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

                    if (solution.selectedOptions[customerId] != optionId) {
                        return false;
                    }

                    if (option.locationId != locationId) {
                        return false;
                    }

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

                // Cada cliente asignado representa una entrega.
                locationLoads[option.locationId]++;
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

        if (!serviceLevelsCanStillBeMet(instance, solution)) {
            return false;
        }

        return true;
    }


    struct InsertionCandidate {
        Solution solution;
        double objective;
        int priority;
    };

    // Genera todas las inserciones factibles del cliente actual
    // y las ordena siguiendo el criterio Greedy.
    vector<InsertionCandidate> generateInsertionCandidates(
        const Instance& instance,
        const Solution& currentSolution,
        int customerId
    ) {
        vector<InsertionCandidate> candidates;

        const int optionCount =
            static_cast<int>(instance.options.size());

        const vector<int>& availableOptions =
            instance.optionsByCustomer[customerId];

        for (int optionId : availableOptions) {

            if (optionId < 0 || optionId >= optionCount) {
                continue;
            }

            const Option& option = instance.options[optionId];

            if (option.customerId != customerId) {
                continue;
            }

            for (int routeIndex = 0;
                 routeIndex < static_cast<int>(currentSolution.routes.size());
                 routeIndex++) {

                const Route& originalRoute =
                    currentSolution.routes[routeIndex];

                for (int position = 0;
                     position <= static_cast<int>(originalRoute.stops.size());
                     position++) {

                    Solution candidate = currentSolution;
                    candidate.selectedOptions[customerId] = optionId;

                    Route& candidateRoute =
                        candidate.routes[routeIndex];

                    Stop newStop;
                    newStop.locationId = option.locationId;
                    newStop.optionIds.push_back(optionId);

                    candidateRoute.stops.insert(
                        candidateRoute.stops.begin() + position,
                        newStop
                    );

                    mergeConsecutiveStops(candidateRoute);

                    if (!isPartialSolutionFeasible(instance, candidate)) {
                        continue;
                    }

                    candidates.push_back({
                        candidate,
                        calculatePartialObjective(instance, candidate),
                        option.priority
                    });
                }
            }

            if (static_cast<int>(currentSolution.routes.size()) <
                instance.vehicleCount) {

                Solution candidate = currentSolution;
                candidate.selectedOptions[customerId] = optionId;

                Route newRoute;
                newRoute.vehicleId =
                    static_cast<int>(candidate.routes.size());

                Stop newStop;
                newStop.locationId = option.locationId;
                newStop.optionIds.push_back(optionId);
                newRoute.stops.push_back(newStop);

                candidate.routes.push_back(newRoute);

                if (isPartialSolutionFeasible(instance, candidate)) {
                    candidates.push_back({
                        candidate,
                        calculatePartialObjective(instance, candidate),
                        option.priority
                    });
                }
            }
        }

        // Orden estable: menor FO primero y, en empate, menor prioridad.
        stable_sort(
            candidates.begin(),
            candidates.end(),
            [](const InsertionCandidate& a,
               const InsertionCandidate& b) {

                if (a.objective != b.objective) {
                    return a.objective < b.objective;
                }

                return a.priority < b.priority;
            }
        );

        return candidates;
    }


    // Búsqueda Greedy con retroceso limitado.
    bool buildGreedyWithBacktracking(
        const Instance& instance,
        const GreedyConfig& config,
        const vector<int>& customerOrder,
        int position,
        Solution& solution,
        int& exploredNodes,
        int& backtracks,
        int& lastFailedCustomer
    ) {
        if (position >= static_cast<int>(customerOrder.size())) {
            return true;
        }

        const int customerId = customerOrder[position];

        vector<InsertionCandidate> candidates =
            generateInsertionCandidates(
                instance,
                solution,
                customerId
            );

        if (candidates.empty()) {
            lastFailedCustomer = customerId;
            return false;
        }

        for (const InsertionCandidate& candidate : candidates) {

            if (exploredNodes >= config.maxSearchNodes ||
                backtracks >= config.maxBacktracks) {
                lastFailedCustomer = customerId;
                return false;
            }

            exploredNodes++;

            Solution nextSolution = candidate.solution;

            if (buildGreedyWithBacktracking(
                instance,
                config,
                customerOrder,
                position + 1,
                nextSolution,
                exploredNodes,
                backtracks,
                lastFailedCustomer
            )) {
                solution = std::move(nextSolution);
                return true;
            }

            // La elección actual no permitió completar la solución:
            // se vuelve atrás y se prueba el siguiente candidato.
            backtracks++;

            if (exploredNodes >= config.maxSearchNodes ||
                backtracks >= config.maxBacktracks) {
                lastFailedCustomer = customerId;
                return false;
            }
        }

        lastFailedCustomer = customerId;
        return false;
    }

}



GreedyResult buildGreedySolution(
    const Instance& instance,
    const GreedyConfig& config
) {

    GreedyResult result;

    const int customerCount =
        static_cast<int>(instance.customers.size());

    if (config.maxSearchNodes <= 0 || config.maxBacktracks <= 0) {
        result.message = "Los limites de busqueda de Greedy deben ser positivos.";
        return result;
    }

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

    // Calcular la ventana temporal mas estrecha disponible para cada cliente.
    // Se utiliza el minimo (DUE - READY) entre las ubicaciones de sus opciones.
    vector<int> minimumWindowByCustomer(customerCount,
                                        numeric_limits<int>::max());

    for (int customerId = 0; customerId < customerCount; customerId++) {
        for (int optionId : instance.optionsByCustomer[customerId]) {
            if (optionId < 0 ||
                optionId >= static_cast<int>(instance.options.size())) {
                continue;
            }

            const int locationId = instance.options[optionId].locationId;
            if (locationId < 0 ||
                locationId >= static_cast<int>(instance.locations.size())) {
                continue;
            }

            const Location& location = instance.locations[locationId];
            const int windowWidth = location.due - location.ready;
            minimumWindowByCustomer[customerId] =
                min(minimumWindowByCustomer[customerId], windowWidth);
        }
    }

    // Preparar el orden de clientes según la estrategia seleccionada.
    // La estrategia por defecto conserva el criterio original:
    // menos opciones primero y, en empate, mayor demanda.
    vector<int> customerOrder(customerCount);

    for (int i = 0; i < customerCount; i++) {
        customerOrder[i] = i;
    }

    sort(
        customerOrder.begin(),
        customerOrder.end(),
        [&](int a, int b) {
            const int optionsA =
                static_cast<int>(instance.optionsByCustomer[a].size());
            const int optionsB =
                static_cast<int>(instance.optionsByCustomer[b].size());
            const int windowA = minimumWindowByCustomer[a];
            const int windowB = minimumWindowByCustomer[b];
            const int demandA = instance.customers[a].demand;
            const int demandB = instance.customers[b].demand;

            switch (config.priorityStrategy) {
                case GreedyPriorityStrategy::WINDOW_THEN_OPTIONS_THEN_DEMAND:
                    if (windowA != windowB) return windowA < windowB;
                    if (optionsA != optionsB) return optionsA < optionsB;
                    return demandA > demandB;

                case GreedyPriorityStrategy::OPTIONS_THEN_WINDOW_THEN_DEMAND:
                    if (optionsA != optionsB) return optionsA < optionsB;
                    if (windowA != windowB) return windowA < windowB;
                    return demandA > demandB;

                case GreedyPriorityStrategy::WINDOW_THEN_DEMAND_THEN_OPTIONS:
                    if (windowA != windowB) return windowA < windowB;
                    if (demandA != demandB) return demandA > demandB;
                    return optionsA < optionsB;

                case GreedyPriorityStrategy::OPTIONS_THEN_DEMAND:
                default:
                    if (optionsA != optionsB) return optionsA < optionsB;
                    return demandA > demandB;
            }
        }
    );

    Solution currentSolution;
    currentSolution.selectedOptions.assign(customerCount, -1);

    int exploredNodes = 0;
    int backtracks = 0;
    int lastFailedCustomer = -1;

    bool foundSolution = buildGreedyWithBacktracking(
        instance,
        config,
        customerOrder,
        0,
        currentSolution,
        exploredNodes,
        backtracks,
        lastFailedCustomer
    );

    if (!foundSolution) {

        result.message =
            "No fue posible construir una solucion factible con Greedy y retroceso limitado.";

        if (lastFailedCustomer >= 0) {
            result.message +=
                " Ultimo cliente sin insercion: " +
                to_string(lastFailedCustomer) + ".";
        }

        result.message +=
            " Nodos explorados: " + to_string(exploredNodes) +
            "/" + to_string(config.maxSearchNodes) +
            ". Retrocesos: " + to_string(backtracks) +
            "/" + to_string(config.maxBacktracks) + ".";

        return result;
    }

    Evaluation evaluation =
        evaluateSolution(instance, currentSolution);

    if (!evaluation.feasible) {

        result.message =
            "Greedy con retroceso construyo una solucion, pero el evaluador detecto incumplimientos.";

        for (const string& violation : evaluation.violations) {
            result.message += "\n- " + violation;
        }

        return result;
    }

    currentSolution.objectiveValue =
        evaluation.objectiveValue;

    result.success = true;
    result.solution = currentSolution;
    result.message =
        "Solucion inicial factible construida correctamente con Greedy y retroceso limitado. "
        "Nodos explorados: " + to_string(exploredNodes) +
        ". Retrocesos: " + to_string(backtracks) + ".";

    return result;
}

GreedyResult buildGreedySolutionWithoutBacktracking(
    const Instance& instance,
    GreedyPriorityStrategy priorityStrategy
) {
    GreedyResult result;

    const int customerCount =
        static_cast<int>(instance.customers.size());

    if (customerCount == 0 ||
        instance.locations.empty() ||
        instance.vehicleCount <= 0 ||
        instance.vehicleCapacity <= 0) {
        result.message =
            "La instancia no contiene datos suficientes para construir una solucion.";
        return result;
    }

    if (static_cast<int>(instance.optionsByCustomer.size()) != customerCount) {
        result.message =
            "La tabla optionsByCustomer no tiene un elemento por cliente.";
        return result;
    }

    vector<int> minimumWindowByCustomer(
        customerCount,
        numeric_limits<int>::max()
    );

    for (int customerId = 0; customerId < customerCount; customerId++) {
        for (int optionId : instance.optionsByCustomer[customerId]) {
            if (optionId < 0 ||
                optionId >= static_cast<int>(instance.options.size())) {
                continue;
            }

            const int locationId = instance.options[optionId].locationId;
            if (locationId < 0 ||
                locationId >= static_cast<int>(instance.locations.size())) {
                continue;
            }

            const Location& location = instance.locations[locationId];
            minimumWindowByCustomer[customerId] = min(
                minimumWindowByCustomer[customerId],
                location.due - location.ready
            );
        }
    }

    vector<int> customerOrder(customerCount);
    for (int customerId = 0; customerId < customerCount; customerId++) {
        customerOrder[customerId] = customerId;
    }

    sort(
        customerOrder.begin(),
        customerOrder.end(),
        [&](int a, int b) {
            const int optionsA =
                static_cast<int>(instance.optionsByCustomer[a].size());
            const int optionsB =
                static_cast<int>(instance.optionsByCustomer[b].size());
            const int windowA = minimumWindowByCustomer[a];
            const int windowB = minimumWindowByCustomer[b];
            const int demandA = instance.customers[a].demand;
            const int demandB = instance.customers[b].demand;

            switch (priorityStrategy) {
                case GreedyPriorityStrategy::WINDOW_THEN_OPTIONS_THEN_DEMAND:
                    if (windowA != windowB) return windowA < windowB;
                    if (optionsA != optionsB) return optionsA < optionsB;
                    return demandA > demandB;

                case GreedyPriorityStrategy::OPTIONS_THEN_WINDOW_THEN_DEMAND:
                    if (optionsA != optionsB) return optionsA < optionsB;
                    if (windowA != windowB) return windowA < windowB;
                    return demandA > demandB;

                case GreedyPriorityStrategy::WINDOW_THEN_DEMAND_THEN_OPTIONS:
                    if (windowA != windowB) return windowA < windowB;
                    if (demandA != demandB) return demandA > demandB;
                    return optionsA < optionsB;

                case GreedyPriorityStrategy::OPTIONS_THEN_DEMAND:
                default:
                    if (optionsA != optionsB) return optionsA < optionsB;
                    return demandA > demandB;
            }
        }
    );

    Solution currentSolution;
    currentSolution.selectedOptions.assign(customerCount, -1);

    int assignedCustomers = 0;

    for (int customerId : customerOrder) {
        vector<InsertionCandidate> candidates =
            generateInsertionCandidates(
                instance,
                currentSolution,
                customerId
            );

        if (candidates.empty()) {
            result.message =
                "Greedy sin backtracking no encontro una insercion factible para el cliente " +
                to_string(customerId) + ". Clientes asignados: " +
                to_string(assignedCustomers) + "/" +
                to_string(customerCount) + ".";
            return result;
        }

        currentSolution = candidates.front().solution;
        assignedCustomers++;
    }

    Evaluation evaluation =
        evaluateSolution(instance, currentSolution);

    if (!evaluation.feasible) {
        result.message =
            "Greedy sin backtracking construyo una solucion, pero el evaluador detecto incumplimientos.";

        for (const string& violation : evaluation.violations) {
            result.message += "\n- " + violation;
        }

        return result;
    }

    currentSolution.objectiveValue = evaluation.objectiveValue;
    result.success = true;
    result.solution = currentSolution;
    result.message =
        "Solucion inicial factible construida con Greedy sin backtracking. Clientes asignados: " +
        to_string(assignedCustomers) + ".";

    return result;
}


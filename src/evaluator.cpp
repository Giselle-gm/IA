
#include "evaluator.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>
#include <vector>

using namespace std;

namespace {
    constexpr double FIXED_VEHICLE_COST = 100000.0;

    void addViolation(Evaluation& evaluation, const string& message) {
        evaluation.violations.push_back(message);
    }

    string locationMessage(int locationId, const string& message) {
        return "Ubicacion " + to_string(locationId) + ": " + message;
    }
}

// --------------------------------------------------
// CALCULO DE VIAJES
// --------------------------------------------------

int travelTime(const Location& from, const Location& to) {
    double dx = from.x - to.x;
    double dy = from.y - to.y;

    double distance = sqrt(dx * dx + dy * dy);

    return static_cast<int>(ceil(10.0 * distance));
}

int travelCost(const Location& from, const Location& to) {
    return travelTime(from, to);
}

// --------------------------------------------------
// EVALUACION DE UNA SOLUCION
// --------------------------------------------------

Evaluation evaluateSolution(
    const Instance& instance,
    const Solution& solution
) {
    Evaluation evaluation;

    const int customerCount =
        static_cast<int>(instance.customers.size());

    const int locationCount =
        static_cast<int>(instance.locations.size());

    const int optionCount =
        static_cast<int>(instance.options.size());

    // Comprobar que exista el depósito.
    if (locationCount == 0) {
        addViolation(evaluation, "La instancia no contiene ubicaciones.");
        evaluation.feasible = false;
        return evaluation;
    }

    // --------------------------------------------------
    // 1. COMPROBAR OPCIONES SELECCIONADAS
    // --------------------------------------------------

    if (static_cast<int>(solution.selectedOptions.size()) != customerCount) {
        addViolation(
            evaluation,
            "selectedOptions no tiene un elemento por cliente."
        );
    }

    vector<int> optionOccurrences(optionCount, 0);

    vector<int> selectedOptionsByCustomer(
        customerCount, -1
    );

    for (int customerId = 0;
         customerId < customerCount &&
         customerId < static_cast<int>(solution.selectedOptions.size());
         customerId++) {

        int optionId = solution.selectedOptions[customerId];

        if (optionId < 0 || optionId >= optionCount) {
            addViolation(
                evaluation,
                "El cliente " + to_string(customerId) +
                " no tiene una opcion valida seleccionada."
            );
            continue;
        }

        const Option& option = instance.options[optionId];

        if (option.customerId != customerId) {
            addViolation(
                evaluation,
                "La opcion seleccionada para el cliente " +
                to_string(customerId) +
                " pertenece a otro cliente."
            );
            continue;
        }

        selectedOptionsByCustomer[customerId] = optionId;

        if (option.locationId < 0 || option.locationId >= locationCount) {
            addViolation(
                evaluation,
                "La opcion " + to_string(optionId) +
                " referencia una ubicacion invalida."
            );
        }

        if (option.priority < 0 ||
            option.priority >= instance.priorityCount) {

            addViolation(
                evaluation,
                "La opcion " + to_string(optionId) +
                " tiene una prioridad invalida."
            );
        }
    }

    // --------------------------------------------------
    // 2. EVALUAR RUTAS Y CAPACIDAD VEHICULAR
    // --------------------------------------------------

    set<int> vehicleIds;

    vector<int> optionRouteOccurrences(optionCount, 0);

    for (const Route& route : solution.routes) {

        // No contar rutas vacías como vehículos utilizados.
        if (route.stops.empty()) {
            addViolation(
                evaluation,
                "Existe una ruta vacia."
            );
            continue;
        }

        evaluation.vehiclesUsed++;

        if (!vehicleIds.insert(route.vehicleId).second) {
            addViolation(
                evaluation,
                "El vehiculo " + to_string(route.vehicleId) +
                " aparece en mas de una ruta."
            );
        }

        int routeLoad = 0;

        vector<bool> customerInRoute(customerCount, false);

        // El vehículo comienza en el depósito.
        int previousLocationId = 0;

        // Tiempo acumulado desde la salida del depósito.
        int currentTime = 0;

        // Recorrer las paradas de la ruta.
        for (const Stop& stop : route.stops) {

            int locationId = stop.locationId;

            if (locationId < 0 || locationId >= locationCount) {
                addViolation(
                    evaluation,
                    "La ruta del vehiculo " +
                    to_string(route.vehicleId) +
                    " contiene una ubicacion invalida."
                );
                continue;
            }

            const Location& location =
                instance.locations[locationId];

            // Viaje hasta la siguiente ubicación.
            const Location& previousLocation =
                instance.locations[previousLocationId];

            int travel = travelTime(previousLocation, location);

            evaluation.travelCost += travel;
            currentTime += travel;

            // Tiempo de llegada.
            int arrivalTime = currentTime;

            // Si llega antes de READY, debe esperar.
            int serviceStart = max(
                arrivalTime,
                location.ready
            );

            // Tiempo de preparación/acceso de la ubicación.
            int serviceDuration = location.serviceTime;

            // Registrar clientes atendidos en esta parada.
            for (int optionId : stop.optionIds) {

                if (optionId < 0 || optionId >= optionCount) {
                    addViolation(
                        evaluation,
                        locationMessage(
                            locationId,
                            "contiene una opcion invalida."
                        )
                    );
                    continue;
                }

                optionRouteOccurrences[optionId]++;

                const Option& option =
                    instance.options[optionId];

                // La opción debe pertenecer a la ubicación de la parada.
                if (option.locationId != locationId) {
                    addViolation(
                        evaluation,
                        "La opcion " + to_string(optionId) +
                        " no pertenece a la ubicacion " +
                        to_string(locationId) + "."
                    );
                }

                // La opción debe coincidir con la seleccionada para el cliente.
                if (option.customerId < 0 ||
                    option.customerId >= customerCount) {

                    addViolation(
                        evaluation,
                        "La opcion " + to_string(optionId) +
                        " tiene un cliente invalido."
                    );
                    continue;
                }

                if (selectedOptionsByCustomer[option.customerId] != optionId) {
                    addViolation(
                        evaluation,
                        "La ruta contiene una opcion distinta a la seleccionada "
                        "para el cliente " +
                        to_string(option.customerId) + "."
                    );
                }

                // Evitar sumar dos veces la demanda de un mismo cliente
                // dentro de una ruta.
                if (!customerInRoute[option.customerId]) {
                    routeLoad +=
                        instance.customers[option.customerId].demand;

                    customerInRoute[option.customerId] = true;
                } else {
                    addViolation(
                        evaluation,
                        "El cliente " + to_string(option.customerId) +
                        " aparece mas de una vez en la misma ruta."
                    );
                }

                // Tiempo de servicio individual de la opción.
                serviceDuration += option.serviceTime;
            }

            // Tiempo de término del servicio.
            int serviceEnd = serviceStart + serviceDuration;

            // Criterio provisional: el servicio completo debe terminar
            // antes o en DUE.
            if (serviceStart < location.ready ||
                serviceEnd > location.due) {

                addViolation(
                    evaluation,
                    locationMessage(
                        locationId,
                        "no cumple su ventana temporal. Inicio: " +
                        to_string(serviceStart) +
                        ", termino: " + to_string(serviceEnd) +
                        ", READY: " + to_string(location.ready) +
                        ", DUE: " + to_string(location.due) + "."
                    )
                );
            }

            // Actualizar el reloj para la siguiente parada.
            currentTime = serviceEnd;
            previousLocationId = locationId;
        }

        // Comprobar capacidad del vehículo.
        if (routeLoad > instance.vehicleCapacity) {
            addViolation(
                evaluation,
                "El vehiculo " + to_string(route.vehicleId) +
                " supera su capacidad. Carga: " +
                to_string(routeLoad) +
                ", capacidad: " +
                to_string(instance.vehicleCapacity) + "."
            );
        }

        // Regreso al depósito.
        const Location& lastLocation =
            instance.locations[previousLocationId];

        const Location& depot =
            instance.locations[0];

        evaluation.travelCost += travelCost(lastLocation, depot);
    }

    // Comprobar cantidad máxima de vehículos.
    if (evaluation.vehiclesUsed > instance.vehicleCount) {
        addViolation(
            evaluation,
            "Se utilizan mas vehiculos que los disponibles."
        );
    }

    // --------------------------------------------------
    // 3. CADA OPCION SELECCIONADA DEBE APARECER EN UNA RUTA
    // --------------------------------------------------

    for (int customerId = 0;
         customerId < customerCount &&
         customerId < static_cast<int>(selectedOptionsByCustomer.size());
         customerId++) {

        int optionId = selectedOptionsByCustomer[customerId];

        if (optionId == -1) {
            continue;
        }

        if (optionRouteOccurrences[optionId] != 1) {
            addViolation(
                evaluation,
                "La opcion " + to_string(optionId) +
                " del cliente " + to_string(customerId) +
                " debe aparecer exactamente una vez en las rutas."
            );
        }
    }

    // --------------------------------------------------
    // 4. CAPACIDAD GLOBAL DE UBICACIONES COMPARTIDAS
    // --------------------------------------------------

    vector<int> locationLoads(locationCount, 0);

    for (int customerId = 0; customerId < customerCount; customerId++) {

        int optionId = selectedOptionsByCustomer[customerId];

        if (optionId < 0) {
            continue;
        }

        const Option& option = instance.options[optionId];

        if (option.locationId < 0 ||
            option.locationId >= locationCount) {
            continue;
        }

        const Location& location =
            instance.locations[option.locationId];

        // TYP = 1 corresponde a una ubicación compartida (SDL).
        if (location.type == 1) {
            locationLoads[option.locationId] +=
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

            addViolation(
                evaluation,
                "La ubicacion compartida " +
                to_string(locationId) +
                " supera su capacidad. Carga: " +
                to_string(locationLoads[locationId]) +
                ", capacidad: " +
                to_string(location.capacity) + "."
            );
        }
    }

    // --------------------------------------------------
    // 5. NIVELES MINIMOS ACUMULATIVOS DE SERVICIO
    // --------------------------------------------------

    for (int priority = 0;
         priority < static_cast<int>(instance.minimumServiceLevel.size());
         priority++) {

        int satisfiedCustomers = 0;

        for (int customerId = 0; customerId < customerCount; customerId++) {

            int optionId = selectedOptionsByCustomer[customerId];

            if (optionId < 0) {
                continue;
            }

            const Option& option = instance.options[optionId];

            // Prioridades acumulativas:
            // para p = 0, contar prioridad 0;
            // para p = 1, contar prioridades 0 y 1.
            if (option.priority <= priority) {
                satisfiedCustomers++;
            }
        }

        double requiredLevel =
            instance.minimumServiceLevel[priority];

        int requiredCustomers = static_cast<int>(
            ceil(requiredLevel * customerCount)
        );

        if (satisfiedCustomers < requiredCustomers) {
            addViolation(
                evaluation,
                "No se cumple el nivel minimo acumulativo para prioridad " +
                to_string(priority) +
                ". Atendidos: " + to_string(satisfiedCustomers) +
                ", minimo requerido: " +
                to_string(requiredCustomers) + "."
            );
        }
    }

    // --------------------------------------------------
    // 6. FUNCION OBJETIVO
    // --------------------------------------------------

    evaluation.fixedCost =
        FIXED_VEHICLE_COST * evaluation.vehiclesUsed;

    evaluation.objectiveValue =
        evaluation.fixedCost + evaluation.travelCost;

    // --------------------------------------------------
    // 7. RESULTADO FINAL
    // --------------------------------------------------

    evaluation.feasible = evaluation.violations.empty();

    return evaluation;
}

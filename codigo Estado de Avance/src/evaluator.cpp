
#include "evaluador.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>
#include <vector>

using namespace std;

namespace {
    //Costo fijo por vehículo utilizado.
    //Usado a modo de prueba
    constexpr double FIXED_VEHICLE_COST = 100000.0;

    void addViolation(Evaluacion& evaluacion, const string& message) {
        evaluacion.violations.push_back(message);
    }

    string locationMessage(int locationId, const string& message) {
        return "Ubicacion " + to_string(locationId) + ": " + message;
    }
}

//Tiempo de viaje entre dos ubicaciones
int travelTime(const Location& from, const Location& to) {
    double dx = from.x - to.x;
    double dy = from.y - to.y;

    double distance = sqrt(dx * dx + dy * dy);

    return static_cast<int>(ceil(10.0 * distance));
}

//Costo de viaje entre dos ubicaciones
int travelCost(const Location& from, const Location& to) {
    return travelTime(from, to);
}


Evaluacion evaluarSolucion(const Instance& instance, const Solucion& solucion) {
    Evaluacion evaluacion;

    const int customerCount = static_cast<int>(instance.customers.size());

    const int locationCount = static_cast<int>(instance.locations.size());

    const int optionCount = static_cast<int>(instance.options.size());

    if (locationCount == 0) {
        addViolation(evaluacion, "La instancia no contiene ubicaciones.");
        evaluacion.factible = false;
        return evaluacion;
    }

    // Verificar que selectedOptions tenga un elemento por cliente.
    if (static_cast<int>(solucion.selectedOptions.size()) != customerCount) {
        addViolation(evaluacion, "selectedOptions no tiene un elemento por cliente.");
    }

    vector<int> optionOccurrences(optionCount, 0);
    vector<int> selectedOptionsByCustomer( customerCount, -1 );

    // Todos los clientes deben tener una opción seleccionada.
    for (int customerId = 0;
         customerId < customerCount &&
         customerId < static_cast<int>(solucion.selectedOptions.size());
         customerId++) {

        int optionId = solucion.selectedOptions[customerId];

        if (optionId < 0 || optionId >= optionCount) {
            addViolation(
                evaluacion,
                "El cliente " + to_string(customerId) +
                " no tiene una opcion valida seleccionada."
            );
            continue;
        }

        const Option& option = instance.options[optionId];
        
        // La opción seleccionada debe pertenecer al cliente correspondiente.
        if (option.customerId != customerId) {
            addViolation(
                evaluacion,
                "La opcion seleccionada para el cliente " +
                to_string(customerId) +
                " pertenece a otro cliente."
            );
            continue;
        }

        selectedOptionsByCustomer[customerId] = optionId;

        // Verificar que la opción seleccionada tenga una ubicación válida.
        if (option.locationId < 0 || option.locationId >= locationCount) {
            addViolation(
                evaluacion,
                "La opcion " + to_string(optionId) +
                " referencia una ubicacion invalida."
            );
        }

        // Verificar que la opción seleccionada tenga una prioridad válida.
        if (option.priority < 0 ||
            option.priority >= instance.priorityCount) {

            addViolation(
                evaluacion,
                "La opcion " + to_string(optionId) +
                " tiene una prioridad invalida."
            );
        }
    }


    set<int> vehicleIds;

    vector<int> optionRouteOccurrences(optionCount, 0);

    for (const Route& route : solucion.routes) {

        //
        if (route.stops.empty()) {
            addViolation(
                evaluacion,
                "Existe una ruta vacia."
            );
            continue;
        }

        evaluacion.vehiculosUsados++;

        if (!vehicleIds.insert(route.vehicleId).second) {
            addViolation(
                evaluacion,
                "El vehiculo " + to_string(route.vehicleId) +
                " aparece en mas de una ruta."
            );
        }

        int routeLoad = 0;

        vector<bool> customerInRoute(customerCount, false);

        int previousLocationId = 0;

        int currentTime = 0;

        for (const Stop& stop : route.stops) {

            if (stop.optionIds.empty()) {
                addViolation(
                    evaluacion,
                    "La ruta del vehiculo " + to_string(route.vehicleId) +
                    " contiene una parada sin opciones de entrega."
                );
                continue;
            }

            int locationId = stop.locationId;

            if (locationId < 0 || locationId >= locationCount) {
                addViolation(
                    evaluacion,
                    "La ruta del vehiculo " +
                    to_string(route.vehicleId) +
                    " contiene una ubicacion invalida."
                );
                continue;
            }

            const Location& location =
                instance.locations[locationId];

            const Location& previousLocation =
                instance.locations[previousLocationId];

            int travel = travelTime(previousLocation, location);

            evaluacion.travelCost += travel;
            currentTime += travel;

            int arrivalTime = currentTime;

            int serviceStart = max(
                arrivalTime,
                location.ready
            );

            // Tiempo de preparación/acceso de la ubicación.
            int serviceDuration = location.serviceTime;

            for (int optionId : stop.optionIds) {

                if (optionId < 0 || optionId >= optionCount) {
                    addViolation(
                        evaluacion,
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
                        evaluacion,
                        "La opcion " + to_string(optionId) +
                        " no pertenece a la ubicacion " +
                        to_string(locationId) + "."
                    );
                }

                // La opción debe coincidir con la seleccionada para el cliente.
                if (option.customerId < 0 ||
                    option.customerId >= customerCount) {

                    addViolation(
                        evaluacion,
                        "La opcion " + to_string(optionId) +
                        " tiene un cliente invalido."
                    );
                    continue;
                }

                if (selectedOptionsByCustomer[option.customerId] != optionId) {
                    addViolation(
                        evaluacion,
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
                        evaluacion,
                        "El cliente " + to_string(option.customerId) +
                        " aparece mas de una vez en la misma ruta."
                    );
                }

                // Tiempo de servicio individual de la opción.
                serviceDuration += option.serviceTime;
            }

            // Tiempo de término del servicio.
            int serviceEnd = serviceStart + serviceDuration;

            // El inicio del servicio debe estar dentro de la ventana.
            if (serviceStart < location.ready ||
                serviceStart > location.due) {

                addViolation(
                    evaluacion,
                    locationMessage(
                        locationId,
                        "no inicia el servicio dentro de su ventana temporal. Inicio: " +
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
                evaluacion,
                "El vehiculo " + to_string(route.vehicleId) +
                " supera su capacidad. Carga: " +
                to_string(routeLoad) +
                ", capacidad: " +
                to_string(instance.vehicleCapacity) + "."
            );
        }

        const Location& lastLocation =
            instance.locations[previousLocationId];

        const Location& depot =
            instance.locations[0];

        evaluacion.travelCost += travelCost(lastLocation, depot);
    }

    if (evaluacion.vehiculosUsados > instance.vehicleCount) {
        addViolation(
            evaluacion,
            "Se utilizan mas vehiculos que los disponibles."
        );
    }


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
                evaluacion,
                "La opcion " + to_string(optionId) +
                " del cliente " + to_string(customerId) +
                " debe aparecer exactamente una vez en las rutas."
            );
        }
    }


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

            addViolation(
                evaluacion,
                "La ubicacion compartida " +
                to_string(locationId) +
                " supera su capacidad. Carga: " +
                to_string(locationLoads[locationId]) +
                ", capacidad: " +
                to_string(location.capacity) + "."
            );
        }
    }

    // 5. NIVELES MINIMOS ACUMULATIVOS DE SERVICIO

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
                evaluacion,
                "No se cumple el nivel minimo acumulativo para prioridad " +
                to_string(priority) +
                ". Atendidos: " + to_string(satisfiedCustomers) +
                ", minimo requerido: " +
                to_string(requiredCustomers) + "."
            );
        }
    }


    evaluacion.fixedCost =
        FIXED_VEHICLE_COST * evaluacion.vehiculosUsados;

    evaluacion.objectiveValue =
        evaluacion.fixedCost + evaluacion.travelCost;


    evaluacion.factible = evaluacion.violations.empty();

    return evaluacion;
}

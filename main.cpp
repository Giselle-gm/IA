#include "parser.h"
#include "greedy.h"
#include "evaluator.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// Muestra el resultado de una solución construida por Greedy.
static void printSolution(
    const Instance& instance,
    const Solution& solution,
    const Evaluation& evaluation
) {
    cout << "\n========== RESULTADO GREEDY ==========\n";

    cout << "Factible: "
         << (evaluation.feasible ? "SI" : "NO") << '\n';

    cout << "Vehiculos utilizados: "
         << evaluation.vehiclesUsed << " / "
         << instance.vehicleCount << '\n';

    cout << fixed << setprecision(2);
    cout << "Costo de viaje: " << evaluation.travelCost << '\n';
    cout << "Costo fijo: " << evaluation.fixedCost << '\n';
    cout << "Funcion objetivo: "
         << evaluation.objectiveValue << '\n';

    // Mostrar las rutas y sus cargas.
    cout << "\n---------- RUTAS ----------\n";

    for (const Route& route : solution.routes) {
        int routeLoad = 0;
        double routeTravelCost = 0.0;
        int previousLocationId = 0;

        cout << "\nVehiculo " << route.vehicleId + 1 << ": Deposito";

        for (const Stop& stop : route.stops) {
            const Location& previous =
                instance.locations[previousLocationId];
            const Location& current =
                instance.locations[stop.locationId];

            routeTravelCost += travelCost(previous, current);

            cout << " -> Ubicacion " << stop.locationId << " [";

            for (size_t i = 0; i < stop.optionIds.size(); ++i) {
                const int optionId = stop.optionIds[i];
                const Option& option = instance.options[optionId];
                const Customer& customer =
                    instance.customers[option.customerId];

                routeLoad += customer.demand;

                if (i > 0) {
                    cout << ", ";
                }

                cout << "O" << option.id
                     << " / Cliente " << option.customerId
                     << " / P" << option.priority;
            }

            cout << "]";
            previousLocationId = stop.locationId;
        }

        routeTravelCost += travelCost(
            instance.locations[previousLocationId],
            instance.locations[0]
        );

        cout << " -> Deposito\n";
        cout << "  Carga: " << routeLoad << " / "
             << instance.vehicleCapacity << '\n';
        cout << "  Costo de ruta: " << routeTravelCost << '\n';
    }

    // Mostrar la opción seleccionada para cada cliente.
    cout << "\n---------- OPCIONES SELECCIONADAS ----------\n";

    for (size_t customerId = 0;
         customerId < solution.selectedOptions.size();
         ++customerId) {

        const int optionId = solution.selectedOptions[customerId];

        if (optionId < 0 ||
            optionId >= static_cast<int>(instance.options.size())) {
            cout << "Cliente " << customerId
                 << ": SIN ASIGNAR\n";
            continue;
        }

        const Option& option = instance.options[optionId];

        cout << "Cliente " << customerId
             << " -> O" << option.id
             << " (Ubicacion " << option.locationId
             << ", prioridad " << option.priority << ")\n";
    }

    // Mostrar el cumplimiento de los niveles mínimos acumulativos.
    cout << "\n---------- NIVELES DE SERVICIO ----------\n";

    const int customerCount =
        static_cast<int>(instance.customers.size());

    for (size_t priority = 0;
         priority < instance.minimumServiceLevel.size();
         ++priority) {

        int satisfiedCustomers = 0;

        for (int optionId : solution.selectedOptions) {
            if (optionId >= 0 &&
                optionId < static_cast<int>(instance.options.size()) &&
                instance.options[optionId].priority
                    <= static_cast<int>(priority)) {
                ++satisfiedCustomers;
            }
        }

        const int requiredCustomers = static_cast<int>(
            ceil(instance.minimumServiceLevel[priority] * customerCount)
        );

        const double achievedPercentage =
            customerCount == 0
                ? 0.0
                : 100.0 * satisfiedCustomers / customerCount;

        cout << "Prioridad acumulada 0-" << priority
             << ": " << satisfiedCustomers << " / "
             << customerCount << " clientes ("
             << achievedPercentage << "%); minimo requerido: "
             << requiredCustomers << " clientes ("
             << instance.minimumServiceLevel[priority] * 100.0
             << "%)\n";
    }

    // Mostrar cualquier incumplimiento detectado por el evaluador.
    if (!evaluation.feasible) {
        cout << "\n---------- VIOLACIONES ----------\n";

        for (const string& violation : evaluation.violations) {
            cout << "- " << violation << '\n';
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Uso: ./bin/vrpdo <archivo_instancia>\n";
        return 1;
    }

    const string filename = argv[1];

    // 1. Leer la instancia.
    Instance instance;

    if (!readInstance(filename, instance)) {
        cerr << "No se pudo leer la instancia: "
             << filename << '\n';
        return 1;
    }

    // 2. Construir la solución inicial con Greedy.
    GreedyResult greedyResult = buildGreedySolution(instance);

    if (!greedyResult.success) {
        cout << "\nNo se pudo construir una solucion Greedy.\n";
        cout << greedyResult.message << '\n';
        return 1;
    }

    cout << "\n" << greedyResult.message << '\n';

    // 3. Evaluar la solución completa.
    const Solution& solution = greedyResult.solution;
    const Evaluation evaluation =
        evaluateSolution(instance, solution);

    // 4. Mostrar resultados y detalles de la solución.
    printSolution(instance, solution, evaluation);

    return evaluation.feasible ? 0 : 1;
}

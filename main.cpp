#include "parser.h"
#include "greedy.h"
#include "evaluator.h"
#include "simulated_annealing.h"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// Muestra el resultado de la mejor solución encontrada por SA.
static void printSolution(
    const Instance& instance,
    const Solution& solution,
    const Evaluation& evaluation
) {
    cout << "\n========== RESULTADO SIMULATED ANNEALING ==========\n";

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

    cout << "\n---------- RUTAS ----------\n";

    for (const Route& route : solution.routes) {
        int routeLoad = 0;
        double routeTravelCost = 0.0;
        int previousLocationId = 0;

        cout << "\nVehiculo " << route.vehicleId + 1 << ": Deposito";

        for (const Stop& stop : route.stops) {
            const Location& previous = instance.locations[previousLocationId];
            const Location& current = instance.locations[stop.locationId];

            routeTravelCost += travelCost(previous, current);
            cout << " -> Ubicacion " << stop.locationId << " [";

            for (size_t i = 0; i < stop.optionIds.size(); ++i) {
                const int optionId = stop.optionIds[i];
                const Option& option = instance.options[optionId];
                const Customer& customer = instance.customers[option.customerId];

                routeLoad += customer.demand;

                if (i > 0) cout << ", ";

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

    cout << "\n---------- OPCIONES SELECCIONADAS ----------\n";

    for (size_t customerId = 0;
         customerId < solution.selectedOptions.size();
         ++customerId) {

        const int optionId = solution.selectedOptions[customerId];

        if (optionId < 0 ||
            optionId >= static_cast<int>(instance.options.size())) {
            cout << "Cliente " << customerId << ": SIN ASIGNAR\n";
            continue;
        }

        const Option& option = instance.options[optionId];

        cout << "Cliente " << customerId
             << " -> O" << option.id
             << " (Ubicacion " << option.locationId
             << ", prioridad " << option.priority << ")\n";
    }

    cout << "\n---------- NIVELES DE SERVICIO ----------\n";

    const int customerCount = static_cast<int>(instance.customers.size());

    for (size_t priority = 0;
         priority < instance.minimumServiceLevel.size();
         ++priority) {

        int satisfiedCustomers = 0;

        for (int optionId : solution.selectedOptions) {
            if (optionId >= 0 &&
                optionId < static_cast<int>(instance.options.size()) &&
                instance.options[optionId].priority <= static_cast<int>(priority)) {
                ++satisfiedCustomers;
            }
        }

        const int requiredCustomers = static_cast<int>(
            ceil(instance.minimumServiceLevel[priority] * customerCount)
        );

        const double achievedPercentage =
            customerCount == 0 ? 0.0 :
            100.0 * satisfiedCustomers / customerCount;

        cout << "Prioridad acumulada 0-" << priority
             << ": " << satisfiedCustomers << " / "
             << customerCount << " clientes ("
             << achievedPercentage << "%); minimo requerido: "
             << requiredCustomers << " clientes ("
             << instance.minimumServiceLevel[priority] * 100.0
             << "%)\n";
    }

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
    constexpr unsigned int RANDOM_SEED = 42;
    const auto computationStart = chrono::steady_clock::now();

    Instance instance;
    if (!readInstance(filename, instance)) {
        cerr << "No se pudo leer la instancia: " << filename << '\n';
        return 1;
    }

    GreedyResult greedyResult = buildGreedySolution(instance);

    if (!greedyResult.success) {
        cout << "\nNo se pudo construir una solucion inicial Greedy.\n";
        cout << greedyResult.message << '\n';
        return 1;
    }

    cout << "\n" << greedyResult.message << '\n';

    const Evaluation initialEvaluation =
        evaluateSolution(instance, greedyResult.solution);

    if (!initialEvaluation.feasible) {
        cerr << "La solucion inicial no supera la evaluacion de factibilidad.\n";
        return 1;
    }

    cout << fixed << setprecision(2);
    cout << "Funcion objetivo inicial: "
         << initialEvaluation.objectiveValue << '\n';

    SAConfig config;
    SAResult saResult = simulatedAnnealing(
        instance,
        greedyResult.solution,
        config
    );

    if (!saResult.success) {
        cerr << "Simulated Annealing no pudo devolver una solucion factible.\n";
        return 1;
    }

    const auto computationEnd = chrono::steady_clock::now();
    const chrono::duration<double> computationTime =
        computationEnd - computationStart;

    cout << "\n========== DATOS DE LA EJECUCION ==========\n";
    cout << "Instancia: " << filename << '\n';
    cout << "Semilla: " << RANDOM_SEED << " (fija en Simulated Annealing)\n";
    cout << "Tiempo de computo: " << fixed << setprecision(3)
         << computationTime.count() << " s\n";

    cout << "Iteraciones ejecutadas: " << saResult.iterationsExecuted << '\n';
    cout << "Movimientos aceptados: " << saResult.acceptedMoves << '\n';
    cout << "Funcion objetivo final: "
         << saResult.bestEvaluation.objectiveValue << '\n';

    const double improvement =
        initialEvaluation.objectiveValue -
        saResult.bestEvaluation.objectiveValue;

    cout << "Mejora absoluta: " << improvement << '\n';

    if (initialEvaluation.objectiveValue > 0.0) {
        cout << "Mejora porcentual: "
             << 100.0 * improvement / initialEvaluation.objectiveValue
             << "%\n";
    }

    printSolution(instance, saResult.bestSolution, saResult.bestEvaluation);

    return 0;
}

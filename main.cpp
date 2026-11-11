#include "parser.h"
#include "greedy.h"
#include "evaluator.h"

#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {

    if (argc < 2) {
        cerr << "Uso: ./bin/vrpdo <archivo_instancia>" << endl;
        return 1;
    }

    Instance instance = parseInstance(argv[1]);

    GreedyResult greedyResult =
        buildGreedySolution(instance);

    if (!greedyResult.success) {
        cout << "No se pudo construir una solucion." << endl;
        cout << greedyResult.message << endl;
        return 1;
    }

    Solution solution = greedyResult.solution;

    // Validar la solución completa con el evaluador general.
    Evaluation evaluation =
        evaluateSolution(instance, solution);

    cout << "\n========== RESULTADO GREEDY ==========\n";

    cout << "Mensaje: " << greedyResult.message << endl;
    cout << "Factible: "
         << (evaluation.feasible ? "SI" : "NO") << endl;

    cout << "Vehiculos utilizados: "
         << evaluation.vehiclesUsed << endl;

    cout << "Costo de viaje: "
         << evaluation.travelCost << endl;

    cout << "Costo fijo: "
         << evaluation.fixedCost << endl;

    cout << "Funcion objetivo: "
         << evaluation.objectiveValue << endl;

    if (!evaluation.feasible) {
        cout << "\nViolaciones:\n";

        for (const string& violation : evaluation.violations) {
            cout << "- " << violation << endl;
        }
    }

    return 0;
}
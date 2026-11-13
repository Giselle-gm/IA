#include "greedy.h"
#include "parser.h"
#include "simulated_annealing.h"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

int main() {
    const fs::path instancesDirectory = "instances";

    if (!fs::exists(instancesDirectory) ||
        !fs::is_directory(instancesDirectory)) {
        cout << "No se encontro el directorio instances/.\n";
        return 1;
    }

    vector<fs::path> instanceFiles;

    for (const fs::directory_entry& entry :
         fs::recursive_directory_iterator(instancesDirectory)) {
        if (entry.is_regular_file()) {
            instanceFiles.push_back(entry.path());
        }
    }

    sort(instanceFiles.begin(), instanceFiles.end());

    int filesReadSuccessfully = 0;
    int greedyFeasible = 0;
    int saSuccessful = 0;
    int saImproved = 0;
    double totalAbsoluteImprovement = 0.0;
    double totalPercentageImprovement = 0.0;

    SAConfig config;

    for (const fs::path& file : instanceFiles) {
        Instance instance;

        if (!readInstance(file.string(), instance)) {
            continue;
        }

        ++filesReadSuccessfully;

        GreedyResult greedyResult = buildGreedySolution(instance);
        if (!greedyResult.success) {
            continue;
        }

        ++greedyFeasible;

        SAResult saResult = simulatedAnnealing(
            instance,
            greedyResult.solution,
            config
        );

        if (!saResult.success) {
            continue;
        }

        ++saSuccessful;

        const double initialObjective =
            saResult.initialEvaluation.objectiveValue;
        const double finalObjective =
            saResult.bestEvaluation.objectiveValue;
        const double absoluteImprovement =
            initialObjective - finalObjective;

        totalAbsoluteImprovement += absoluteImprovement;

        if (initialObjective > 0.0) {
            totalPercentageImprovement +=
                100.0 * absoluteImprovement / initialObjective;
        }

        if (absoluteImprovement > 0.0) {
            ++saImproved;
        }
    }

    cout << fixed << setprecision(2);
    cout << "\n========== EVALUACION DE SIMULATED ANNEALING ==========\n";
    cout << "Archivos encontrados: " << instanceFiles.size() << '\n';
    cout << "Archivos leidos exitosamente: "
         << filesReadSuccessfully << '\n';
    cout << "Soluciones iniciales factibles con Greedy: "
         << greedyFeasible << '\n';
    cout << "Instancias procesadas exitosamente por SA: "
         << saSuccessful << '\n';
    cout << "Instancias mejoradas por SA: "
         << saImproved << '\n';

    if (saSuccessful > 0) {
        cout << "Mejora absoluta promedio: "
             << totalAbsoluteImprovement / saSuccessful << '\n';
        cout << "Mejora porcentual promedio: "
             << totalPercentageImprovement / saSuccessful << "%\n";
    } else {
        cout << "No hubo instancias procesadas exitosamente por SA.\n";
    }

    return 0;
}

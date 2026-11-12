#include "greedy.h"
#include "parser.h"
#include "simulated_annealing.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
namespace fs = std::filesystem;
using Clock = chrono::steady_clock;

struct StrategySummary {
    GreedyStrategy strategy;
    int runs = 0;
    int greedyFeasible = 0;
    int saSuccessful = 0;
    int saImproved = 0;
    double totalInitialObjective = 0.0;
    double totalFinalObjective = 0.0;
    double totalAbsoluteImprovement = 0.0;
    double totalPercentageImprovement = 0.0;
    double totalGreedySeconds = 0.0;
    double totalSaSeconds = 0.0;
};

string csvEscape(const string& value) {
    string escaped = "\"";
    for (char character : value) {
        if (character == '"') escaped += '"';
        escaped += character;
    }
    escaped += '"';
    return escaped;
}

int main() {
    const fs::path root = "instances";
    if (!fs::exists(root) || !fs::is_directory(root)) {
        cerr << "No se encontro el directorio instances/.\n";
        return 1;
    }

    vector<fs::path> files;
    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (entry.is_regular_file()) files.push_back(entry.path());
    }
    sort(files.begin(), files.end());

    const vector<GreedyStrategy> strategies = {
        GreedyStrategy::RANDOM_ORDER,
        GreedyStrategy::FEWEST_OPTIONS_FIRST,
        GreedyStrategy::MOST_CONSTRAINED_FIRST
    };
    const vector<unsigned int> seeds = {42, 43, 44};

    GreedyConfig greedyConfig;
    greedyConfig.maxRestarts = 100;
    greedyConfig.restrictedCandidateListSize = 5;

    SAConfig saConfig;

    ofstream output("greedy_sa_comparison.csv");
    if (!output) {
        cerr << "No se pudo crear greedy_sa_comparison.csv.\n";
        return 2;
    }

    output << "instancia,estrategia,semilla,greedy_factible,intentos,"
              "greedy_tiempo_segundos,objetivo_inicial,costo_viaje_inicial,"
              "costo_fijo_inicial,vehiculos_inicial,sa_exitoso,"
              "iteraciones_sa,movimientos_aceptados,sa_tiempo_segundos,"
              "objetivo_final,costo_viaje_final,costo_fijo_final,"
              "vehiculos_final,mejora_absoluta,mejora_porcentual\n";

    vector<StrategySummary> summaries;
    for (GreedyStrategy strategy : strategies) {
        StrategySummary summary;
        summary.strategy = strategy;
        summaries.push_back(summary);
    }

    int filesRead = 0;
    int readErrors = 0;

    for (const fs::path& file : files) {
        Instance instance;
        if (!readInstance(file.string(), instance)) {
            ++readErrors;
            continue;
        }
        ++filesRead;

        for (size_t strategyIndex = 0;
             strategyIndex < strategies.size(); ++strategyIndex) {
            const GreedyStrategy strategy = strategies[strategyIndex];
            StrategySummary& summary = summaries[strategyIndex];

            for (unsigned int seed : seeds) {
                ++summary.runs;
                greedyConfig.randomSeed = seed;

                const auto greedyStart = Clock::now();
                const GreedyResult greedy =
                    buildGreedySolution(instance, greedyConfig);
                const double greedySeconds = chrono::duration<double>(
                    Clock::now() - greedyStart
                ).count();
                summary.totalGreedySeconds += greedySeconds;

                output << csvEscape(file.generic_string()) << ","
                       << csvEscape(greedyStrategyName(strategy)) << ","
                       << seed << "," << (greedy.success ? 1 : 0) << ","
                       << greedy.restartsAttempted << ","
                       << fixed << setprecision(6) << greedySeconds << ",";

                if (!greedy.success) {
                    output << string(13, ',') << "\n";
                    continue;
                }

                ++summary.greedyFeasible;

                const auto saStart = Clock::now();
                const SAResult sa = simulatedAnnealing(
                    instance, greedy.solution, saConfig
                );
                const double saSeconds = chrono::duration<double>(
                    Clock::now() - saStart
                ).count();
                summary.totalSaSeconds += saSeconds;

                const Evaluation& initial = sa.initialEvaluation;
                const Evaluation& final = sa.bestEvaluation;

                output << fixed << setprecision(2)
                       << initial.objectiveValue << ","
                       << initial.travelCost << ","
                       << initial.fixedCost << ","
                       << initial.vehiclesUsed << ","
                       << (sa.success ? 1 : 0) << ","
                       << sa.iterationsExecuted << ","
                       << sa.acceptedMoves << ","
                       << setprecision(6) << saSeconds << ",";

                if (!sa.success) {
                    output << ",,,,,\n";
                    continue;
                }

                ++summary.saSuccessful;

                const double absoluteImprovement =
                    initial.objectiveValue - final.objectiveValue;
                const double percentageImprovement =
                    initial.objectiveValue > 0.0
                    ? 100.0 * absoluteImprovement / initial.objectiveValue
                    : 0.0;

                summary.totalInitialObjective += initial.objectiveValue;
                summary.totalFinalObjective += final.objectiveValue;
                summary.totalAbsoluteImprovement += absoluteImprovement;
                summary.totalPercentageImprovement += percentageImprovement;

                if (absoluteImprovement > 1e-9) ++summary.saImproved;

                output << fixed << setprecision(2)
                       << final.objectiveValue << ","
                       << final.travelCost << ","
                       << final.fixedCost << ","
                       << final.vehiclesUsed << ","
                       << absoluteImprovement << ","
                       << percentageImprovement << "\n";
            }
        }
    }

    cout << fixed << setprecision(3);
    cout << "\n========== COMPARACION GREEDY + SIMULATED ANNEALING ==========\n";
    cout << "Archivos encontrados: " << files.size() << '\n';
    cout << "Archivos leidos: " << filesRead << '\n';
    cout << "Errores de lectura: " << readErrors << '\n';
    cout << "Semillas por estrategia: " << seeds.size() << '\n';
    cout << "Reinicios Greedy maximos: " << greedyConfig.maxRestarts << '\n';
    cout << "RCL: " << greedyConfig.restrictedCandidateListSize << '\n';
    cout << "Iteraciones SA configuradas: " << saConfig.iterations << "\n\n";

    for (const StrategySummary& summary : summaries) {
        cout << "--- " << greedyStrategyName(summary.strategy) << " ---\n";
        cout << "Ejecuciones: " << summary.runs << '\n';
        cout << "Greedy factible: " << summary.greedyFeasible
             << "/" << summary.runs << '\n';
        cout << "SA exitoso: " << summary.saSuccessful << '\n';
        cout << "Soluciones mejoradas por SA: " << summary.saImproved << '\n';

        if (summary.saSuccessful > 0) {
            const double count = summary.saSuccessful;
            cout << "Objetivo inicial promedio: "
                 << summary.totalInitialObjective / count << '\n';
            cout << "Objetivo final promedio: "
                 << summary.totalFinalObjective / count << '\n';
            cout << "Mejora absoluta promedio: "
                 << summary.totalAbsoluteImprovement / count << '\n';
            cout << "Mejora porcentual promedio: "
                 << summary.totalPercentageImprovement / count << "%\n";
            cout << "Tiempo promedio Greedy: "
                 << summary.totalGreedySeconds / summary.runs << " s\n";
            cout << "Tiempo promedio SA: "
                 << summary.totalSaSeconds / summary.saSuccessful << " s\n";
        }
        cout << '\n';
    }

    cout << "Resultados detallados: greedy_sa_comparison.csv\n";
    return readErrors == 0 ? 0 : 3;
}

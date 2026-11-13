#include "greedy.h"
#include "parser.h"

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
    int factibleRuns = 0;
    long long totalAttempts = 0;
    long long failedRuns = 0;
    long long assignedOnFailures = 0;
    double totalSeconds = 0.0;
};

string joinIds(const vector<int>& values) {
    string result;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i) result += " ";
        result += to_string(values[i]);
    }
    return result;
}

int main() {
    const fs::path root = "instances";
    if (!fs::exists(root) || !fs::is_directory(root)) {
        cerr << "No se encontro el directorio instances/.\n";
        return 1;
    }

    vector<fs::path> files;
    for (const auto& entry : fs::recursive_directory_iterator(root))
        if (entry.is_regular_file()) files.push_back(entry.path());
    sort(files.begin(), files.end());

    const vector<GreedyStrategy> strategies = {
        GreedyStrategy::RANDOM_ORDER,
        GreedyStrategy::FEWEST_OPTIONS_FIRST,
        GreedyStrategy::MOST_CONSTRAINED_FIRST
    };
    const vector<unsigned int> seeds = {42, 43, 44};

    ofstream details("greedy_restart_details.csv");
    ofstream runs("greedy_strategy_runs.csv");
    if (!details || !runs) {
        cerr << "No se pudieron crear los archivos CSV.\n";
        return 2;
    }

    details << "instancia,estrategia,semilla,intento,orden_clientes,opciones_seleccionadas,"
               "clientes_asignados,factible,motivo_fallo,tiempo_segundos\n";
    runs << "instancia,estrategia,semilla,exito,intentos,clientes_asignados,tiempo_total_segundos\n";

    vector<StrategySummary> summaries;
    for (GreedyStrategy strategy : strategies)
        summaries.push_back({strategy});

    int readCount = 0;
    int readErrors = 0;

    for (const fs::path& file : files) {
        Instance instance;
        if (!readInstance(file.string(), instance)) {
            ++readErrors;
            continue;
        }
        ++readCount;

        for (size_t strategyIndex = 0; strategyIndex < strategies.size(); ++strategyIndex) {
            const GreedyStrategy strategy = strategies[strategyIndex];
            StrategySummary& summary = summaries[strategyIndex];

            for (unsigned int seed : seeds) {
                GreedyConfig config;
                config.maxRestarts = 100;
                config.restrictedCandidateListSize = 5;
                config.randomSeed = seed;
                config.strategy = strategy;

                const auto start = Clock::now();
                const GreedyResult result = buildGreedySolucion(instance, config);
                const double totalSeconds = chrono::duration<double>(
                    Clock::now() - start
                ).count();

                ++summary.runs;
                summary.totalAttempts += result.restartsAttempted;
                summary.totalSeconds += totalSeconds;
                if (result.success) {
                    ++summary.factibleRuns;
                } else {
                    ++summary.failedRuns;
                    summary.assignedOnFailures += result.assignedCustomers;
                }

                runs << '"' << file.generic_string() << ","
                     << greedyStrategyName(strategy) << "," << seed << ","
                     << (result.success ? 1 : 0) << ","
                     << result.restartsAttempted << ","
                     << result.assignedCustomers << ","
                     << fixed << setprecision(6) << totalSeconds << "\n";

                for (const RestartStats& stats : result.restartStats) {
                    details << '"' << file.generic_string() << ","
                            << greedyStrategyName(strategy) << ","
                            << seed << "," << stats.attempt << ","
                            << joinIds(stats.customerOrder) << ","
                            << joinIds(stats.selectedOptionOrder) << ","
                            << stats.assignedCustomers << ","
                            << (stats.factible ? 1 : 0) << ","
                            << stats.failureReason << ","
                            << fixed << setprecision(6)
                            << stats.elapsedSeconds << "\n";
                }
            }
        }
    }

    cout << fixed << setprecision(3);
    cout << "\n========== COMPARACION DE ESTRATEGIAS GREEDY ==========\n";
    cout << "Archivos encontrados: " << files.size() << '\n';
    cout << "Archivos leidos: " << readCount << '\n';
    cout << "Errores de lectura: " << readErrors << '\n';
    cout << "Semillas por instancia y estrategia: " << seeds.size() << '\n';
    cout << "Reinicios maximos por ejecucion: 100\n";
    cout << "RCL: 5 candidatos\n\n";

    for (const StrategySummary& summary : summaries) {
        const double successRate = summary.runs == 0 ? 0.0 :
            100.0 * summary.factibleRuns / summary.runs;
        const double averageAttempts = summary.runs == 0 ? 0.0 :
            static_cast<double>(summary.totalAttempts) / summary.runs;
        const double averageTime = summary.runs == 0 ? 0.0 :
            summary.totalSeconds / summary.runs;

        cout << "--- " << greedyStrategyName(summary.strategy) << " ---\n";
        cout << "Ejecuciones: " << summary.runs << '\n';
        cout << "Soluciones factibles: " << summary.factibleRuns
             << "/" << summary.runs << " (" << successRate << "%)\n";
        cout << "Ejecuciones fallidas: " << summary.failedRuns << '\n';
        cout << "Intentos promedio: " << averageAttempts << '\n';
        cout << "Tiempo promedio: " << averageTime << " s\n\n";
    }

    cout << "Detalle por reinicio: greedy_restart_details.csv\n";
    cout << "Resumen por ejecucion: greedy_strategy_runs.csv\n";
    return readErrors == 0 ? 0 : 3;
}

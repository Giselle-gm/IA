#include "greedy.h"
#include "parser.h"
#include "simulated_annealing.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <vector>

using namespace std;
namespace fs = std::filesystem;
using Clock = chrono::steady_clock;

struct InstanceStrategyStats {
    int repetitions = 0;
    int greedySuccess = 0;
    int saSuccess = 0;
    int improved = 0;
    long long greedyAttempts = 0;
    long long greedyFailedRestarts = 0;
    long long saIterations = 0;
    long long saAcceptedMoves = 0;
    double greedySeconds = 0.0;
    double saSeconds = 0.0;
    double initialObjective = 0.0;
    double finalObjective = 0.0;
    double absoluteImprovement = 0.0;
    double percentageImprovement = 0.0;
};

struct StrategyTotals : InstanceStrategyStats {
    int instances = 0;
};

static string csv(const string& value) {
    string result = "\"";
    for (char c : value) {
        if (c == '"') result += '"';
        result += c;
    }
    return result + "\"";
}

int main() {
    const fs::path root = "instances";
    if (!fs::exists(root) || !fs::is_directory(root)) {
        cerr << "No se encontro el directorio instances/.\\n";
        return 1;
    }

    vector<fs::path> files;
    for (const auto& entry : fs::recursive_directory_iterator(root))
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
            files.push_back(entry.path());
    sort(files.begin(), files.end());

    const vector<GreedyStrategy> strategies = {
        GreedyStrategy::FEWEST_OPTIONS_FIRST,
        GreedyStrategy::MOST_CONSTRAINED_FIRST
    };
    const vector<unsigned int> seeds = {42, 43, 44};

    GreedyConfig greedyConfig;
    greedyConfig.maxRestarts = 100;
    greedyConfig.restrictedCandidateListSize = 5;

    SAConfig saConfig;
    const string runFile = "greedy_sa_runs.csv";
    const string instanceFile = "greedy_sa_by_instance.csv";
    const string restartFile = "greedy_sa_restarts.csv";

    ofstream runs(runFile), byInstance(instanceFile), restarts(restartFile);
    if (!runs || !byInstance || !restarts) {
        cerr << "No se pudieron crear los CSV de resultados.\\n";
        return 2;
    }

    runs << "instancia,estrategia,repeticion,semilla_greedy,semilla_sa,"
            "greedy_factible,clientes_asignados,intentos_greedy,"
            "reinicios_fallidos,greedy_segundos,sa_ejecutado,iteraciones_sa,"
            "iteraciones_configuradas,movimientos_aceptados,sa_segundos,"
            "objetivo_inicial,costo_viaje_inicial,costo_fijo_inicial,"
            "vehiculos_inicial,objetivo_final,costo_viaje_final,costo_fijo_final,"
            "vehiculos_final,mejora_absoluta,mejora_porcentual\\n";

    byInstance << "instancia,estrategia,repeticiones,greedy_factibles,sa_exitosos,"
                  "soluciones_mejoradas,intentos_greedy_totales,"
                  "reinicios_fallidos_totales,iteraciones_sa_totales,"
                  "iteraciones_sa_promedio,movimientos_aceptados_totales,"
                  "greedy_segundos_totales,sa_segundos_totales,"
                  "objetivo_inicial_promedio,objetivo_final_promedio,"
                  "mejora_absoluta_promedio,mejora_porcentual_promedio\\n";

    restarts << "instancia,estrategia,repeticion,semilla_greedy,intento,"
                "clientes_asignados,factible,tiempo_segundos,motivo_fallo\\n";

    map<pair<string, GreedyStrategy>, InstanceStrategyStats> perInstance;
    map<GreedyStrategy, StrategyTotals> totals;
    int filesRead = 0, readErrors = 0;
    long long totalGreedyAttempts = 0, totalFailedRestarts = 0;
    long long totalSaIterations = 0, totalSaAccepted = 0;
    int totalRuns = 0, totalGreedySuccess = 0, totalSaSuccess = 0, totalImproved = 0;
    double totalGreedySeconds = 0.0, totalSaSeconds = 0.0;

    for (const fs::path& file : files) {
        Instance instance;
        if (!readInstance(file.string(), instance)) {
            ++readErrors;
            continue;
        }
        ++filesRead;
        const string instanceName = file.generic_string();

        for (GreedyStrategy strategy : strategies) {
            auto& stats = perInstance[{instanceName, strategy}];
            auto& total = totals[strategy];
            ++total.instances;

            for (size_t repetition = 0; repetition < seeds.size(); ++repetition) {
                const unsigned int seed = seeds[repetition];
                ++totalRuns;
                ++stats.repetitions;
                ++total.repetitions;
                greedyConfig.randomSeed = seed;
                greedyConfig.strategy = strategy;
                saConfig.randomSeed = seed;

                const auto greedyStart = Clock::now();
                const GreedyResult greedy = buildGreedySolucion(instance, greedyConfig);
                const double greedySeconds = chrono::duration<double>(
                    Clock::now() - greedyStart).count();

                int failedRestarts = 0;
                for (const RestartStats& item : greedy.restartStats) {
                    if (!item.factible) ++failedRestarts;
                    restarts << csv(instanceName) << ','
                             << csv(greedyStrategyName(strategy)) << ','
                             << repetition + 1 << ',' << seed << ','
                             << item.attempt << ',' << item.assignedCustomers << ','
                             << (item.factible ? 1 : 0) << ','
                             << fixed << setprecision(6) << item.elapsedSeconds << ','
                             << csv(item.failureReason) << '\\n';
                }

                stats.greedyAttempts += greedy.restartsAttempted;
                stats.greedySeconds += greedySeconds;
                stats.greedyFailedRestarts += failedRestarts;
                total.greedyAttempts += greedy.restartsAttempted;
                total.greedyFailedRestarts += failedRestarts;
                total.greedySeconds += greedySeconds;
                totalGreedyAttempts += greedy.restartsAttempted;
                totalFailedRestarts += failedRestarts;
                totalGreedySeconds += greedySeconds;

                runs << csv(instanceName) << ','
                     << csv(greedyStrategyName(strategy)) << ','
                     << repetition + 1 << ',' << seed << ',' << seed << ','
                     << (greedy.success ? 1 : 0) << ','
                     << greedy.assignedCustomers << ','
                     << greedy.restartsAttempted << ',' << failedRestarts << ','
                     << fixed << setprecision(6) << greedySeconds << ',';

                if (!greedy.success) {
                    runs << "0,0," << saConfig.iterations
                         << ",0,0,,,,,,,,\\n";
                    continue;
                }

                ++stats.greedySuccess;
                ++total.greedySuccess;
                ++totalGreedySuccess;

                const auto saStart = Clock::now();
                const SAResult sa = simulatedAnnealing(instance, greedy.solucion, saConfig);
                const double saSeconds = chrono::duration<double>(
                    Clock::now() - saStart).count();

                stats.saSeconds += saSeconds;
                total.saSeconds += saSeconds;
                totalSaSeconds += saSeconds;
                stats.saIterations += sa.iterationsExecuted;
                total.saIterations += sa.iterationsExecuted;
                totalSaIterations += sa.iterationsExecuted;
                stats.saAcceptedMoves += sa.acceptedMoves;
                total.saAcceptedMoves += sa.acceptedMoves;
                totalSaAccepted += sa.acceptedMoves;

                const Evaluacion& initial = sa.initialEvaluacion;
                const Evaluacion& final = sa.bestEvaluacion;
                const double improvement = initial.objectiveValue - final.objectiveValue;
                const double percentage = initial.objectiveValue > 0.0
                    ? 100.0 * improvement / initial.objectiveValue : 0.0;

                runs << (sa.success ? 1 : 0) << ','
                     << sa.iterationsExecuted << ',' << saConfig.iterations << ','
                     << sa.acceptedMoves << ',' << fixed << setprecision(6)
                     << saSeconds << ',' << fixed << setprecision(2)
                     << initial.objectiveValue << ',' << initial.travelCost << ','
                     << initial.fixedCost << ',' << initial.vehiculosUsados << ','
                     << final.objectiveValue << ',' << final.travelCost << ','
                     << final.fixedCost << ',' << final.vehiculosUsados << ','
                     << improvement << ',' << percentage << '\\n';

                if (sa.success) {
                    ++stats.saSuccess;
                    ++total.saSuccess;
                    ++totalSaSuccess;
                    stats.initialObjective += initial.objectiveValue;
                    stats.finalObjective += final.objectiveValue;
                    stats.absoluteImprovement += improvement;
                    stats.percentageImprovement += percentage;
                    total.initialObjective += initial.objectiveValue;
                    total.finalObjective += final.objectiveValue;
                    total.absoluteImprovement += improvement;
                    total.percentageImprovement += percentage;
                    if (improvement > 1e-9) {
                        ++stats.improved;
                        ++total.improved;
                        ++totalImproved;
                    }
                }
            }
        }
    }

    for (const auto& entry : perInstance) {
        const string& name = entry.first.first;
        const GreedyStrategy strategy = entry.first.second;
        const auto& s = entry.second;
        const double n = s.saSuccess > 0 ? s.saSuccess : 1.0;
        byInstance << csv(name) << ',' << csv(greedyStrategyName(strategy)) << ','
                   << s.repetitions << ',' << s.greedySuccess << ',' << s.saSuccess
                   << ',' << s.improved << ',' << s.greedyAttempts << ','
                   << s.greedyFailedRestarts << ',' << s.saIterations << ','
                   << (s.saSuccess ? static_cast<double>(s.saIterations) / s.saSuccess : 0.0)
                   << ',' << s.saAcceptedMoves << ',' << fixed << setprecision(6)
                   << s.greedySeconds << ',' << s.saSeconds << ','
                   << fixed << setprecision(2) << s.initialObjective / n << ','
                   << s.finalObjective / n << ',' << s.absoluteImprovement / n << ','
                   << s.percentageImprovement / n << '\\n';
    }

    cout << fixed << setprecision(3);
    cout << "\\n========== EXPERIMENTO GREEDY + SA POR INSTANCIA ==========\\n";
    cout << "Archivos encontrados: " << files.size() << '\\n';
    cout << "Archivos leidos: " << filesRead << '\\n';
    cout << "Errores de lectura: " << readErrors << '\\n';
    cout << "Estrategias: 2 (sin Greedy aleatorio)\\n";
    cout << "Repeticiones por instancia y estrategia: " << seeds.size() << '\\n';
    cout << "Ejecuciones previstas: " << filesRead * strategies.size() * seeds.size() << '\\n';
    cout << "Ejecuciones realizadas: " << totalRuns << '\\n';
    cout << "Greedy factibles: " << totalGreedySuccess << '\\n';
    cout << "Reinicios Greedy totales: " << totalGreedyAttempts << '\\n';
    cout << "Reinicios fallidos totales: " << totalFailedRestarts << '\\n';
    cout << "Iteraciones SA ejecutadas en total: " << totalSaIterations << '\\n';
    cout << "Iteraciones SA configuradas por ejecucion: " << saConfig.iterations << '\\n';
    cout << "Movimientos aceptados por SA en total: " << totalSaAccepted << '\\n';
    cout << "SA exitosos: " << totalSaSuccess << '\\n';
    cout << "Soluciones mejoradas: " << totalImproved << '\\n';
    cout << "Tiempo Greedy total: " << totalGreedySeconds << " s\\n";
    cout << "Tiempo SA total: " << totalSaSeconds << " s\\n\\n";

    for (GreedyStrategy strategy : strategies) {
        const auto& s = totals[strategy];
        cout << "--- " << greedyStrategyName(strategy) << " ---\\n";
        cout << "Ejecuciones: " << s.repetitions << '\\n';
        cout << "Greedy factibles: " << s.greedySuccess << '\\n';
        cout << "Reinicios totales: " << s.greedyAttempts << '\\n';
        cout << "Reinicios fallidos: " << s.greedyFailedRestarts << '\\n';
        cout << "Iteraciones SA ejecutadas: " << s.saIterations << '\\n';
        cout << "Iteraciones SA promedio por ejecucion exitosa: "
             << (s.saSuccess ? static_cast<double>(s.saIterations) / s.saSuccess : 0.0) << '\\n';
        cout << "Movimientos aceptados: " << s.saAcceptedMoves << '\\n';
        cout << "Soluciones mejoradas: " << s.improved << '\\n';
        cout << "Tiempo total Greedy: " << s.greedySeconds << " s\\n";
        cout << "Tiempo total SA: " << s.saSeconds << " s\\n\\n";
    }

    cout << "CSV ejecuciones individuales: " << runFile << '\\n';
    cout << "CSV resumen por instancia: " << instanceFile << '\\n';
    cout << "CSV detalle de reinicios: " << restartFile << '\\n';
    return readErrors == 0 ? 0 : 3;
}

#include "greedy.h"
#include "parser.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
namespace fs = std::filesystem;
using Clock = chrono::steady_clock;

namespace {

struct Strategy {
    const char* name;
    GreedyPriorityStrategy priority;
};

struct StrategyStats {
    string name;
    int feasible = 0;
    int failed = 0;
    long long totalNodes = 0;
    long long totalBacktracks = 0;
    double seconds = 0.0;
    vector<string> failedFiles;
};

int extractNumberAfter(const string& message, const string& marker) {
    const size_t position = message.find(marker);
    if (position == string::npos) return -1;

    const size_t start = position + marker.size();
    size_t end = start;
    while (end < message.size() && message[end] >= '0' && message[end] <= '9') {
        ++end;
    }
    if (end == start) return -1;
    return stoi(message.substr(start, end - start));
}

} // namespace

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

    const vector<Strategy> strategies = {
        {"Actual: opciones -> demanda",
         GreedyPriorityStrategy::OPTIONS_THEN_DEMAND},
        {"Ventana -> opciones -> demanda",
         GreedyPriorityStrategy::WINDOW_THEN_OPTIONS_THEN_DEMAND},
        {"Opciones -> ventana -> demanda",
         GreedyPriorityStrategy::OPTIONS_THEN_WINDOW_THEN_DEMAND},
        {"Ventana -> demanda -> opciones",
         GreedyPriorityStrategy::WINDOW_THEN_DEMAND_THEN_OPTIONS}
    };

    vector<Instance> instances;
    vector<string> instanceNames;
    int readErrors = 0;

    for (const fs::path& file : files) {
        Instance instance;
        if (!readInstance(file.string(), instance)) {
            ++readErrors;
            continue;
        }
        instances.push_back(std::move(instance));
        instanceNames.push_back(file.string());
    }

    cout << "\n========== COMPARACION DE PRIORIDADES GREEDY ==========\n";
    cout << "Archivos encontrados: " << files.size() << '\n';
    cout << "Instancias leidas: " << instances.size() << '\n';
    cout << "Errores de lectura: " << readErrors << '\n';
    cout << "Limite de nodos por instancia: 20000\n";
    cout << "Limite de retrocesos por instancia: 5000\n";
    cout << "Las cuatro estrategias usan exactamente los mismos limites.\n";

    vector<StrategyStats> results;
    results.reserve(strategies.size());

    const auto totalStart = Clock::now();

    for (const Strategy& strategy : strategies) {
        StrategyStats stats;
        stats.name = strategy.name;

        GreedyConfig config;
        config.maxSearchNodes = 20000;
        config.maxBacktracks = 5000;
        config.priorityStrategy = strategy.priority;

        const auto start = Clock::now();

        for (size_t i = 0; i < instances.size(); ++i) {
            GreedyResult result = buildGreedySolution(instances[i], config);

            if (result.success) {
                ++stats.feasible;
            } else {
                ++stats.failed;
                stats.failedFiles.push_back(instanceNames[i]);
            }

            const int nodes = extractNumberAfter(
                result.message, "Nodos explorados: ");
            const int backtracks = extractNumberAfter(
                result.message, "Retrocesos: ");

            if (nodes >= 0) stats.totalNodes += nodes;
            if (backtracks >= 0) stats.totalBacktracks += backtracks;
        }

        stats.seconds = chrono::duration<double>(Clock::now() - start).count();

        cout << "\n--- " << stats.name << " ---\n";
        cout << "Soluciones factibles: " << stats.feasible
             << " / " << instances.size() << '\n';
        cout << "Instancias no resueltas: " << stats.failed << '\n';
        cout << "Nodos explorados acumulados: " << stats.totalNodes << '\n';
        cout << "Retrocesos acumulados: " << stats.totalBacktracks << '\n';
        cout << "Tiempo: " << fixed << setprecision(3)
             << stats.seconds << " s\n";

        results.push_back(std::move(stats));
    }

    cout << "\n========== RESUMEN COMPARATIVO ==========\n";
    cout << left << setw(38) << "Estrategia"
         << right << setw(12) << "Factibles"
         << setw(12) << "Fallidas"
         << setw(14) << "Nodos"
         << setw(14) << "Retrocesos"
         << setw(12) << "Tiempo(s)" << '\n';

    for (const StrategyStats& stats : results) {
        cout << left << setw(38) << stats.name
             << right << setw(12) << stats.feasible
             << setw(12) << stats.failed
             << setw(14) << stats.totalNodes
             << setw(14) << stats.totalBacktracks
             << setw(12) << fixed << setprecision(2) << stats.seconds << '\n';
    }

    cout << "\n========== INSTANCIAS NO RESUELTAS POR ESTRATEGIA ==========\n";
    for (const StrategyStats& stats : results) {
        cout << "\n" << stats.name << ":\n";
        if (stats.failedFiles.empty()) {
            cout << "  Ninguna.\n";
        } else {
            for (const string& name : stats.failedFiles) {
                cout << "  " << name << '\n';
            }
        }
    }

    cout << "\nTiempo total de comparacion: " << fixed << setprecision(3)
         << chrono::duration<double>(Clock::now() - totalStart).count()
         << " s\n";

    cout << "\nNota: encontrar mas soluciones con un mismo presupuesto indica "
            "una mejor construccion para este conjunto, pero no demuestra "
            "que las instancias restantes sean inviables.\n";

    return readErrors == 0 ? 0 : 2;
}

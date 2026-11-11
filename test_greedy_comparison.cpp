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

int main() {
    const fs::path root = "instances";

    if (!fs::exists(root) || !fs::is_directory(root)) {
        cerr << "No se encontro el directorio instances/.\\n";
        return 1;
    }

    vector<fs::path> files;
    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path());
        }
    }
    sort(files.begin(), files.end());

    int readCount = 0;
    int readErrors = 0;
    int backtrackingSuccesses = 0;
    int simpleGreedySuccesses = 0;
    int bothSuccesses = 0;
    double backtrackingSeconds = 0.0;
    double simpleGreedySeconds = 0.0;

    for (const fs::path& file : files) {
        Instance instance;
        if (!readInstance(file.string(), instance)) {
            ++readErrors;
            continue;
        }
        ++readCount;

        const auto backtrackingStart = Clock::now();
        const GreedyResult withBacktracking =
            buildGreedySolution(instance);
        backtrackingSeconds +=
            chrono::duration<double>(Clock::now() - backtrackingStart).count();

        const auto simpleStart = Clock::now();
        const GreedyResult withoutBacktracking =
            buildGreedySolutionWithoutBacktracking(instance);
        simpleGreedySeconds +=
            chrono::duration<double>(Clock::now() - simpleStart).count();

        if (withBacktracking.success) {
            ++backtrackingSuccesses;
        }
        if (withoutBacktracking.success) {
            ++simpleGreedySuccesses;
        }
        if (withBacktracking.success && withoutBacktracking.success) {
            ++bothSuccesses;
        }
    }

    cout << fixed << setprecision(3);
    cout << "\\n========== COMPARACION DE GREEDY ==========\\n";
    cout << "Archivos encontrados: " << files.size() << '\\n';
    cout << "Archivos leidos: " << readCount << '\\n';
    cout << "Errores de lectura: " << readErrors << '\\n';
    cout << "\\n";
    cout << left << setw(32) << "Algoritmo"
         << right << setw(18) << "Soluciones"
         << setw(18) << "Tiempo (s)" << '\\n';
    cout << left << setw(32) << "Greedy con Backtracking"
         << right << setw(18) << backtrackingSuccesses
         << setw(18) << backtrackingSeconds << '\\n';
    cout << left << setw(32) << "Greedy sin Backtracking"
         << right << setw(18) << simpleGreedySuccesses
         << setw(18) << simpleGreedySeconds << '\\n';
    cout << "\\nInstancias resueltas por ambos: " << bothSuccesses << '\\n';

    return readErrors == 0 ? 0 : 2;
}

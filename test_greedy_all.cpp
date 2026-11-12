#include "greedy.h"
#include "greedy_sequential.h"
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

struct Stats {
    int feasible = 0;
    double seconds = 0.0;
};

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

    Stats withBacktracking, withoutBacktracking, sequential;
    int readCount = 0;
    int readErrors = 0;
    int allThree = 0;
    int backtrackingAndSimple = 0;
    int backtrackingAndSequential = 0;
    int simpleAndSequential = 0;

    for (const fs::path& file : files) {
        Instance instance;
        if (!readInstance(file.string(), instance)) {
            ++readErrors;
            continue;
        }
        ++readCount;

        auto start = Clock::now();
        const GreedyResult a = buildGreedySolution(instance);
        withBacktracking.seconds += chrono::duration<double>(Clock::now() - start).count();

        start = Clock::now();
        const GreedyResult b = buildGreedySolutionWithoutBacktracking(instance);
        withoutBacktracking.seconds += chrono::duration<double>(Clock::now() - start).count();

        start = Clock::now();
        const GreedyResult c = buildGreedySequentialSolution(instance);
        sequential.seconds += chrono::duration<double>(Clock::now() - start).count();

        withBacktracking.feasible += a.success;
        withoutBacktracking.feasible += b.success;
        sequential.feasible += c.success;
        allThree += a.success && b.success && c.success;
        backtrackingAndSimple += a.success && b.success;
        backtrackingAndSequential += a.success && c.success;
        simpleAndSequential += b.success && c.success;
    }

    cout << fixed << setprecision(3);
    cout << "\n========== COMPARACION DE TRES GREEDY ==========\n";
    cout << "Archivos encontrados: " << files.size() << '\n';
    cout << "Archivos leidos: " << readCount << '\n';
    cout << "Errores de lectura: " << readErrors << "\n\n";
    cout << left << setw(36) << "Algoritmo"
         << right << setw(14) << "Factibles"
         << setw(16) << "Tiempo (s)" << '\n';
    cout << left << setw(36) << "Greedy con Backtracking"
         << right << setw(14) << withBacktracking.feasible
         << setw(16) << withBacktracking.seconds << '\n';
    cout << left << setw(36) << "Greedy sin Backtracking"
         << right << setw(14) << withoutBacktracking.feasible
         << setw(16) << withoutBacktracking.seconds << '\n';
    cout << left << setw(36) << "Greedy secuencial (nuevo)"
         << right << setw(14) << sequential.feasible
         << setw(16) << sequential.seconds << "\n\n";

    cout << "Resueltas por Backtracking y sin Backtracking: "
         << backtrackingAndSimple << '\n';
    cout << "Resueltas por Backtracking y secuencial: "
         << backtrackingAndSequential << '\n';
    cout << "Resueltas por sin Backtracking y secuencial: "
         << simpleAndSequential << '\n';
    cout << "Resueltas por los tres algoritmos: " << allThree << '\n';

    return readErrors == 0 ? 0 : 2;
}

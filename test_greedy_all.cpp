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
        cerr << "No se encontro el directorio instances/.\n";
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
    int feasibleCount = 0;
    long long totalRestarts = 0;
    double totalSeconds = 0.0;

    for (const fs::path& file : files) {
        Instance instance;
        if (!readInstance(file.string(), instance)) {
            ++readErrors;
            continue;
        }

        ++readCount;
        const auto start = Clock::now();
        const GreedyResult result = buildGreedySolution(instance);
        totalSeconds += chrono::duration<double>(
            Clock::now() - start
        ).count();

        totalRestarts += result.restartsAttempted;
        feasibleCount += result.success;
    }

    cout << fixed << setprecision(3);
    cout << "\n========== RESULTADOS GREEDY CON REINICIOS ==========\n";
    cout << "Archivos encontrados: " << files.size() << '\n';
    cout << "Archivos leidos: " << readCount << '\n';
    cout << "Errores de lectura: " << readErrors << '\n';
    cout << "Soluciones iniciales factibles: "
         << feasibleCount << "/" << readCount << '\n';
    cout << "Reinicios acumulados: " << totalRestarts << '\n';
    cout << "Tiempo total: " << totalSeconds << " s\n";

    return readErrors == 0 ? 0 : 2;
}

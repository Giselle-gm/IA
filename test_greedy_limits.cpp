#include "greedy.h"
#include "parser.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace std;
namespace fs = std::filesystem;
using Clock = chrono::steady_clock;

struct FailedInstance {
    fs::path path;
    Instance instance;
};

int main() {
    const fs::path instancesDirectory = "instances";
    if (!fs::exists(instancesDirectory) || !fs::is_directory(instancesDirectory)) {
        cerr << "No se encontro el directorio instances/.\n";
        return 1;
    }

    vector<fs::path> files;
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(instancesDirectory)) {
        if (entry.is_regular_file()) files.push_back(entry.path());
    }
    sort(files.begin(), files.end());

    const GreedyConfig defaultConfig;
    vector<FailedInstance> failures;
    int readSuccessfully = 0;
    int readErrors = 0;
    int baselineFeasible = 0;

    for (const fs::path& file : files) {
        Instance instance;
        if (!readInstance(file.string(), instance)) {
            ++readErrors;
            continue;
        }
        ++readSuccessfully;
        GreedyResult result = buildGreedySolution(instance, defaultConfig);
        if (result.success) ++baselineFeasible;
        else failures.push_back({file, std::move(instance)});
    }

    cout << "\n========== PRUEBA DE LIMITES DE GREEDY ==========\n";
    cout << "Archivos encontrados: " << files.size() << '\n';
    cout << "Archivos leidos exitosamente: " << readSuccessfully << '\n';
    cout << "Errores de lectura: " << readErrors << '\n';
    cout << "Factibles con configuracion original (nodos="
         << defaultConfig.maxSearchNodes << ", retrocesos="
         << defaultConfig.maxBacktracks << "): " << baselineFeasible << '\n';
    cout << "Instancias que se vuelven a probar: " << failures.size() << "\n";

    const vector<int> backtrackLimits = {10000, 20000, 50000};
    for (int limit : backtrackLimits) {
        GreedyConfig config;
        config.maxSearchNodes = defaultConfig.maxSearchNodes;
        config.maxBacktracks = limit;

        int resolved = 0;
        vector<string> resolvedFiles;
        const auto start = Clock::now();

        for (const FailedInstance& failed : failures) {
            GreedyResult result = buildGreedySolution(failed.instance, config);
            if (result.success) {
                ++resolved;
                resolvedFiles.push_back(failed.path.string());
            }
        }

        const chrono::duration<double> elapsed = Clock::now() - start;
        cout << "\n--- Limite de retrocesos: " << limit << " ---\n";
        cout << "Soluciones encontradas entre las " << failures.size()
             << " fallidas: " << resolved << '\n';
        cout << "Tiempo de prueba: " << fixed << setprecision(3)
             << elapsed.count() << " s\n";
        if (!resolvedFiles.empty()) {
            cout << "Instancias resueltas:\n";
            for (const string& name : resolvedFiles) cout << "  " << name << '\n';
        }
    }

    cout << "\nNota: los resultados no demuestran que las instancias restantes "
            "sean inviables.\n";
    return 0;
}

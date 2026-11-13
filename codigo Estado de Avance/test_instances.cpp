#include "greedy.h"
#include "parser.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
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
    int factibleSolucions = 0;

    for (const fs::path& file : instanceFiles) {
        Instance instance;

        if (!readInstance(file.string(), instance)) {
            continue;
        }

        ++filesReadSuccessfully;

        GreedyResult result = buildGreedySolucion(instance);

        if (result.success) {
            ++factibleSolucions;
        }
    }

    cout << "\n========== RESUMEN DE INSTANCIAS ==========\n";
    cout << "Archivos encontrados: "
         << instanceFiles.size() << '\n';
    cout << "Archivos leidos exitosamente: "
         << filesReadSuccessfully << '\n';
    cout << "Archivos con solucion inicial factible (Greedy): "
         << factibleSolucions << '\n';

    return 0;
}

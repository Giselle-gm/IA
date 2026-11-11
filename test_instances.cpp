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
        return 0;
    }

    vector<fs::path> instanceFiles;

    // Recorrer recursivamente todos los subdirectorios de instances.
    for (const fs::directory_entry& entry :
         fs::recursive_directory_iterator(instancesDirectory)) {

        if (entry.is_regular_file()) {
            instanceFiles.push_back(entry.path());
        }
    }

    sort(instanceFiles.begin(), instanceFiles.end());

    int feasibleSolutions = 0;

    for (const fs::path& file : instanceFiles) {
        Instance instance;

        // Los archivos que no se puedan leer se omiten.
        if (!readInstance(file.string(), instance)) {
            continue;
        }

        GreedyResult result = buildGreedySolution(instance);

        if (result.success) {
            ++feasibleSolutions;
        }
    }

    // No se muestran nombres ni detalles de las instancias.
    // Si ninguna tuvo solución factible, no se imprime nada.
    if (feasibleSolutions > 0) {
        cout << "Se encontraron soluciones iniciales factibles en "
             << feasibleSolutions << " instancia(s).\n";
    }

    return 0;
}

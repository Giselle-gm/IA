#include "greedy.h"
#include "parser.h"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

namespace {

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

struct FailureInfo {
    fs::path file;
    int customers = 0;
    int options = 0;
    int vehicles = 0;
    int vehicleCapacity = 0;
    long long totalDemand = 0;
    int maxDemand = 0;
    int minOptions = 0;
    int maxOptions = 0;
    int failedCustomer = -1;
    int exploredNodes = -1;
    int backtracks = -1;
    string category;
};

}

int main() {
    const fs::path instancesDirectory = "instances";

    if (!fs::exists(instancesDirectory) ||
        !fs::is_directory(instancesDirectory)) {
        cerr << "No se encontro el directorio instances/.\n";
        return 1;
    }

    vector<fs::path> files;
    for (const fs::directory_entry& entry :
         fs::recursive_directory_iterator(instancesDirectory)) {
        if (entry.is_regular_file()) files.push_back(entry.path());
    }
    sort(files.begin(), files.end());

    int readSuccessfully = 0;
    int greedyFeasible = 0;
    int readFailed = 0;
    vector<FailureInfo> failures;

    for (const fs::path& file : files) {
        Instance instance;

        if (!readInstance(file.string(), instance)) {
            ++readFailed;
            continue;
        }
        ++readSuccessfully;

        GreedyResult greedy = buildGreedySolution(instance);
        if (greedy.success) {
            ++greedyFeasible;
            continue;
        }

        FailureInfo info;
        info.file = file;
        info.customers = static_cast<int>(instance.customers.size());
        info.options = static_cast<int>(instance.options.size());
        info.vehicles = instance.vehicleCount;
        info.vehicleCapacity = instance.vehicleCapacity;

        for (const Customer& customer : instance.customers) {
            info.totalDemand += customer.demand;
            info.maxDemand = max(info.maxDemand, customer.demand);
        }

        if (!instance.optionsByCustomer.empty()) {
            info.minOptions = static_cast<int>(instance.optionsByCustomer[0].size());
            info.maxOptions = info.minOptions;
            for (const auto& options : instance.optionsByCustomer) {
                const int count = static_cast<int>(options.size());
                info.minOptions = min(info.minOptions, count);
                info.maxOptions = max(info.maxOptions, count);
            }
        }

        info.failedCustomer =
            extractNumberAfter(greedy.message, "Ultimo cliente sin insercion: ");
        info.exploredNodes =
            extractNumberAfter(greedy.message, "Nodos explorados: ");
        info.backtracks =
            extractNumberAfter(greedy.message, "Retrocesos: ");

        if (info.maxDemand > info.vehicleCapacity) {
            info.category = "DEMANDA_INDIVIDUAL_SUPERA_CAPACIDAD";
        } else if (info.totalDemand >
                   static_cast<long long>(info.vehicles) * info.vehicleCapacity) {
            info.category = "DEMANDA_TOTAL_SUPERA_FLOTA";
        } else if (info.exploredNodes >= 20000 || info.backtracks >= 5000) {
            info.category = "LIMITE_DE_BUSQUEDA_ALCANZADO";
        } else {
            info.category = "SIN_INSERCION_FACTIBLE_EN_BUSQUEDA";
        }

        failures.push_back(info);
    }

    int individualDemandFailures = 0;
    int fleetDemandFailures = 0;
    int searchLimitFailures = 0;
    int noInsertionFailures = 0;

    cout << "\n========== DIAGNOSTICO DE GREEDY: INSTANCIAS FALLIDAS ==========\n";
    cout << "Archivos encontrados: " << files.size() << '\n';
    cout << "Archivos leidos exitosamente: " << readSuccessfully << '\n';
    cout << "Errores de lectura: " << readFailed << '\n';
    cout << "Soluciones Greedy factibles: " << greedyFeasible << '\n';
    cout << "Instancias sin solucion inicial encontrada: " << failures.size() << "\n\n";

    cout << left
         << setw(34) << "Instancia"
         << right
         << setw(5) << "N"
         << setw(6) << "O"
         << setw(5) << "K"
         << setw(8) << "Q"
         << setw(9) << "Demanda"
         << setw(8) << "MaxDem"
         << setw(7) << "MinOp"
         << setw(7) << "MaxOp"
         << setw(7) << "CliFall"
         << setw(8) << "Nodos"
         << setw(8) << "Back"
         << "  Diagnostico\n";

    for (const FailureInfo& f : failures) {
        cout << left << setw(34) << f.file.string()
             << right << setw(5) << f.customers
             << setw(6) << f.options
             << setw(5) << f.vehicles
             << setw(8) << f.vehicleCapacity
             << setw(9) << f.totalDemand
             << setw(8) << f.maxDemand
             << setw(7) << f.minOptions
             << setw(7) << f.maxOptions
             << setw(7) << f.failedCustomer
             << setw(8) << f.exploredNodes
             << setw(8) << f.backtracks
             << "  " << f.category << '\n';

        if (f.category == "DEMANDA_INDIVIDUAL_SUPERA_CAPACIDAD") {
            ++individualDemandFailures;
        } else if (f.category == "DEMANDA_TOTAL_SUPERA_FLOTA") {
            ++fleetDemandFailures;
        } else if (f.category == "LIMITE_DE_BUSQUEDA_ALCANZADO") {
            ++searchLimitFailures;
        } else {
            ++noInsertionFailures;
        }
    }

    cout << "\n========== RESUMEN DE DIAGNOSTICOS ==========\n";
    cout << "Demanda individual superior a Q: "
         << individualDemandFailures << '\n';
    cout << "Demanda total superior a K*Q: "
         << fleetDemandFailures << '\n';
    cout << "Limite de nodos o retrocesos alcanzado: "
         << searchLimitFailures << '\n';
    cout << "Sin insercion factible antes de alcanzar limites: "
         << noInsertionFailures << '\n';

    return 0;
}

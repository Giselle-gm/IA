#include "greedy.h"
#include "parser.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

using namespace std;
namespace fs = std::filesystem;
using Clock = chrono::steady_clock;

namespace {

struct InstanceStats {
    string file;
    int customers = 0;
    int options = 0;
    int locations = 0;
    int sharedLocations = 0;
    int vehicles = 0;
    int vehicleCapacity = 0;
    long long totalDemand = 0;
    int maxDemand = 0;
    double averageDemand = 0.0;
    double fleetLoadRatio = 0.0;
    int minOptions = 0;
    int maxOptions = 0;
    double averageOptions = 0.0;
    int minWindow = 0;
    int maxWindow = 0;
    double averageWindow = 0.0;
    int priority0Options = 0;
    int priority1Options = 0;
    int exploredNodes = -1;
    int backtracks = -1;
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

InstanceStats collectStats(const string& filename, const Instance& instance,
                           const string& message = "") {
    InstanceStats s;
    s.file = filename;
    s.customers = static_cast<int>(instance.customers.size());
    s.options = static_cast<int>(instance.options.size());
    s.locations = static_cast<int>(instance.locations.size());
    s.vehicles = instance.vehicleCount;
    s.vehicleCapacity = instance.vehicleCapacity;

    for (const Customer& customer : instance.customers) {
        s.totalDemand += customer.demand;
        s.maxDemand = max(s.maxDemand, customer.demand);
    }
    if (s.customers > 0) {
        s.averageDemand = static_cast<double>(s.totalDemand) / s.customers;
    }
    const long long fleetCapacity =
        static_cast<long long>(s.vehicles) * s.vehicleCapacity;
    if (fleetCapacity > 0) {
        s.fleetLoadRatio = static_cast<double>(s.totalDemand) / fleetCapacity;
    }

    if (!instance.optionsByCustomer.empty()) {
        s.minOptions = static_cast<int>(instance.optionsByCustomer[0].size());
        s.maxOptions = s.minOptions;
        int totalOptionsPerCustomer = 0;
        for (const auto& available : instance.optionsByCustomer) {
            const int count = static_cast<int>(available.size());
            s.minOptions = min(s.minOptions, count);
            s.maxOptions = max(s.maxOptions, count);
            totalOptionsPerCustomer += count;
        }
        if (s.customers > 0) {
            s.averageOptions =
                static_cast<double>(totalOptionsPerCustomer) / s.customers;
        }
    }

    vector<int> windowWidths;
    for (const Location& location : instance.locations) {
        if (location.id == 0) continue;
        windowWidths.push_back(location.due - location.ready);
        if (location.type == 1) ++s.sharedLocations;
    }
    if (!windowWidths.empty()) {
        s.minWindow = *min_element(windowWidths.begin(), windowWidths.end());
        s.maxWindow = *max_element(windowWidths.begin(), windowWidths.end());
        s.averageWindow = accumulate(windowWidths.begin(), windowWidths.end(), 0.0)
                          / windowWidths.size();
    }

    for (const Option& option : instance.options) {
        if (option.priority == 0) ++s.priority0Options;
        if (option.priority == 1) ++s.priority1Options;
    }

    s.exploredNodes = extractNumberAfter(message, "Nodos explorados: ");
    s.backtracks = extractNumberAfter(message, "Retrocesos: ");
    return s;
}

void printGroupSummary(const string& title, const vector<InstanceStats>& group) {
    cout << "\n--- " << title << " (" << group.size() << ") ---\n";
    if (group.empty()) return;

    auto average = [&](auto getter) {
        double total = 0.0;
        for (const auto& s : group) total += getter(s);
        return total / group.size();
    };
    auto minimum = [&](auto getter) {
        int value = getter(group.front());
        for (const auto& s : group) value = min(value, getter(s));
        return value;
    };
    auto maximum = [&](auto getter) {
        int value = getter(group.front());
        for (const auto& s : group) value = max(value, getter(s));
        return value;
    };

    cout << fixed << setprecision(2);
    cout << "Clientes (promedio): " << average([](const auto& s) { return s.customers; }) << '\n';
    cout << "Opciones totales (promedio): " << average([](const auto& s) { return s.options; }) << '\n';
    cout << "Ubicaciones compartidas (promedio): "
         << average([](const auto& s) { return s.sharedLocations; }) << '\n';
    cout << "Demanda total (promedio): " << average([](const auto& s) { return s.totalDemand; }) << '\n';
    cout << "Demanda maxima individual (promedio): "
         << average([](const auto& s) { return s.maxDemand; }) << '\n';
    cout << "Uso de capacidad total K*Q (promedio): "
         << 100.0 * average([](const auto& s) { return s.fleetLoadRatio; }) << "%\n";
    cout << "Opciones por cliente (promedio): "
         << average([](const auto& s) { return s.averageOptions; }) << '\n';
    cout << "Ventana temporal promedio por instancia: "
         << average([](const auto& s) { return s.averageWindow; }) << '\n';
    cout << "Prioridad 0 (opciones promedio): "
         << average([](const auto& s) { return s.priority0Options; }) << '\n';
    cout << "Prioridad 1 (opciones promedio): "
         << average([](const auto& s) { return s.priority1Options; }) << '\n';
    cout << "Rango clientes: " << minimum([](const auto& s) { return s.customers; })
         << " - " << maximum([](const auto& s) { return s.customers; }) << '\n';
    cout << "Rango opciones por cliente: "
         << minimum([](const auto& s) { return s.minOptions; }) << " minimo global; "
         << maximum([](const auto& s) { return s.maxOptions; }) << " maximo global\n";
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

    GreedyConfig originalConfig;
    GreedyConfig extendedConfig;
    extendedConfig.maxSearchNodes = originalConfig.maxSearchNodes;
    extendedConfig.maxBacktracks = 50000;

    vector<InstanceStats> solved;
    vector<InstanceStats> remaining;
    int readErrors = 0;
    int readCount = 0;
    int baselineFeasible = 0;
    const auto totalStart = Clock::now();

    for (const fs::path& path : files) {
        Instance instance;
        if (!readInstance(path.string(), instance)) {
            ++readErrors;
            continue;
        }
        ++readCount;

        GreedyResult first = buildGreedySolution(instance, originalConfig);
        if (first.success) {
            ++baselineFeasible;
            solved.push_back(collectStats(path.string(), instance));
            continue;
        }

        GreedyResult extended = buildGreedySolution(instance, extendedConfig);
        if (extended.success) {
            solved.push_back(collectStats(path.string(), instance));
        } else {
            remaining.push_back(
                collectStats(path.string(), instance, extended.message)
            );
        }
    }

    const chrono::duration<double> elapsed = Clock::now() - totalStart;

    cout << "\n========== CARACTERIZACION DE INSTANCIAS NO RESUELTAS ==========\n";
    cout << "Archivos encontrados: " << files.size() << '\n';
    cout << "Archivos leidos: " << readCount << '\n';
    cout << "Errores de lectura: " << readErrors << '\n';
    cout << "Resueltas con configuracion original: " << baselineFeasible << '\n';
    cout << "Resueltas considerando hasta 50.000 retrocesos: " << solved.size() << '\n';
    cout << "No resueltas con 20.000 nodos y 50.000 retrocesos: "
         << remaining.size() << '\n';
    cout << "Tiempo total: " << fixed << setprecision(3) << elapsed.count() << " s\n";

    printGroupSummary("Instancias resueltas con el presupuesto ampliado", solved);
    printGroupSummary("Instancias que siguen sin solucion", remaining);

    cout << "\n========== DETALLE DE LAS INSTANCIAS RESTANTES ==========\n";
    cout << left << setw(34) << "Instancia"
         << right << setw(4) << "N"
         << setw(5) << "O"
         << setw(4) << "K"
         << setw(5) << "Q"
         << setw(8) << "Demanda"
         << setw(7) << "MaxD"
         << setw(6) << "MinO"
         << setw(6) << "MaxO"
         << setw(7) << "SDL"
         << setw(8) << "Nodos"
         << setw(8) << "Back" << '\n';

    for (const auto& s : remaining) {
        cout << left << setw(34) << s.file
             << right << setw(4) << s.customers
             << setw(5) << s.options
             << setw(4) << s.vehicles
             << setw(5) << s.vehicleCapacity
             << setw(8) << s.totalDemand
             << setw(7) << s.maxDemand
             << setw(6) << s.minOptions
             << setw(6) << s.maxOptions
             << setw(7) << s.sharedLocations
             << setw(8) << s.exploredNodes
             << setw(8) << s.backtracks << '\n';
    }

    cout << "\nNota: no encontrar una solucion dentro de estos limites no demuestra "
            "que la instancia sea inviable.\n";
    return 0;
}

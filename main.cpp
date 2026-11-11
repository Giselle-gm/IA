
#include <iostream>
#include "include/instance.h"
#include "include/parser.h"

using namespace std;

void printInstanceSummary(const Instance& instance) {
    cout << "\n========== RESUMEN DE INSTANCIA ==========\n";

    cout << "Vehiculos: " << instance.vehicleCount << endl;
    cout << "Capacidad vehicular: "
         << instance.vehicleCapacity << endl;

    cout << "Clientes: " << instance.customers.size() << endl;
    cout << "Ubicaciones: " << instance.locations.size() << endl;
    cout << "Opciones: " << instance.options.size() << endl;

    cout << "\n--- Opciones por cliente ---\n";

    for (const Customer& customer : instance.customers) {
        cout << "Cliente " << customer.id
             << " | Demanda: " << customer.demand
             << " | Opciones: ";

        for (int optionId : instance.optionsByCustomer[customer.id]) {
            cout << "O" << optionId << " ";
        }

        cout << endl;
    }

    cout << "\n--- Opciones por ubicacion ---\n";

    for (const Location& location : instance.locations) {
        cout << "Ubicacion " << location.id << ": ";

        for (int optionId : instance.optionsByLocation[location.id]) {
            cout << "O" << optionId << " ";
        }

        cout << endl;
    }

    cout << "\n==========================================\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Uso: " << argv[0]
             << " <archivo_instancia>" << endl;
        return 1;
    }

    Instance instance;

    if (!readInstance(argv[1], instance)) {
        return 1;
    }

    printInstanceSummary(instance);

    return 0;
}

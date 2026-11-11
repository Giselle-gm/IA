
#ifndef INSTANCE_H
#define INSTANCE_H

#include <vector>

using namespace std;

// Solicitud o cliente
struct Customer {
    int id;
    int demand;
};

// Ubicacion de entrega o deposito
struct Location {
    int id;
    double x, y;

    int capacity;
    int ready;
    int due;
    int type;
    int serviceTime;
};

// Alternativa de entrega de una solicitud
struct Option {
    int id;
    int locationId;
    int customerId;
    int priority;
    int serviceTime;
    int cost;
};


// Datos completos de una instancia
struct Instance {
    int vehicleCount;
    int vehicleCapacity;

    int priorityCount;
    int typeCount;

    vector<Customer> customers;
    vector<Location> locations;
    vector<Option> options;

    // Indices auxiliares
    // optionsByCustomer[n]: opciones disponibles para el cliente n
    // optionsByLocation[l]: opciones asociadas a la ubicacion l
    vector<vector<int>> optionsByCustomer;
    vector<vector<int>> optionsByLocation;

    // Niveles mínimos acumulativos de servicio.
    // Índice 0: prioridad 0.
    // Índice 1: prioridades 0 y 1.
    vector<double> minimumServiceLevel = {0.80, 0.90};
};

#endif

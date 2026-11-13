
#ifndef SOLUCION_H
#define SOLUCION_H

#include <vector>

struct Stop {
    int locationId = -1;

    // Opciones atendidas durante esta parada.
    std::vector<int> optionIds;
};

struct Route {
    int vehicleId = -1;

    // Las ubicaciones 0 (depósito) de inicio y término
    // se consideran implícitas.
    std::vector<Stop> stops;
};

struct Solucion {
    // Índice: ID del cliente.
    // Valor: ID de la opción seleccionada.
    // -1 significa que todavía no se ha seleccionado.
    std::vector<int> selectedOptions;

    std::vector<Route> routes;

    double objectiveValue = 0.0;
};

#endif

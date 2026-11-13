#include "parser.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace std;

namespace {

bool nextLine(ifstream& file, string& line) {
    while (getline(file, line)) {
        if (line.find_first_not_of(" \t\r\n") != string::npos) {
            return true;
        }
    }
    return false;
}

void skipSectionHeader(ifstream& file) {
    string line;

    if (!nextLine(file, line)) {
        throw runtime_error("Falta una cabecera de seccion.");
    }
}

void prepareLine(istringstream& stream, const string& line) {
    stream.clear();
    stream.str(line);
}

} // namespace

bool readInstance(const string& filename, Instance& instance) {
    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "Error: no se pudo abrir " << filename << endl;
        return false;
    }

    instance = Instance{};

    try {
        string line;
        istringstream stream;


        if (!nextLine(file, line)) {
            throw runtime_error("Archivo vacio.");
        }

        if (!nextLine(file, line)) {
            throw runtime_error(
                "Falta el encabezado de parametros generales."
            );
        }

        if (!nextLine(file, line)) {
            throw runtime_error("Faltan los parametros generales.");
        }

        prepareLine(stream, line);

        int locationCount = 0;
        int customerCount = 0;
        int optionCount = 0;

        if (!(stream >> instance.vehicleCount
                     >> instance.vehicleCapacity
                     >> locationCount
                     >> customerCount
                     >> optionCount
                     >> instance.priorityCount
                     >> instance.typeCount)) {
            throw runtime_error("Parametros generales invalidos.");
        }

        if (instance.vehicleCount <= 0 ||
            instance.vehicleCapacity <= 0 ||
            locationCount <= 0 ||
            customerCount <= 0 ||
            optionCount <= 0 ||
            instance.priorityCount <= 0 ||
            instance.typeCount <= 0) {
            throw runtime_error(
                "Los parametros y las dimensiones deben ser positivos."
            );
        }

        instance.customers.resize(customerCount);
        instance.locations.resize(locationCount);
        instance.options.resize(optionCount);

        instance.optionsByCustomer.assign(
            customerCount, vector<int>()
        );

        instance.optionsByLocation.assign(
            locationCount, vector<int>()
        );


        skipSectionHeader(file);

        for (int i = 0; i < customerCount; ++i) {
            if (!nextLine(file, line)) {
                throw runtime_error("Faltan datos de clientes.");
            }

            prepareLine(stream, line);

            Customer customer{};

            if (!(stream >> customer.id >> customer.demand)) {
                throw runtime_error("Fila de cliente invalida.");
            }

            if (customer.id < 0 || customer.id >= customerCount) {
                throw runtime_error("ID de cliente fuera de rango.");
            }

            if (customer.demand < 0) {
                throw runtime_error("Demanda negativa.");
            }

            instance.customers[customer.id] = customer;
        }


        skipSectionHeader(file);

        for (int i = 0; i < locationCount; ++i) {
            if (!nextLine(file, line)) {
                throw runtime_error("Faltan datos de ubicaciones.");
            }

            prepareLine(stream, line);

            Location location{};

            if (!(stream >> location.id
                         >> location.x
                         >> location.y
                         >> location.capacity
                         >> location.ready
                         >> location.due
                         >> location.type
                         >> location.serviceTime)) {
                throw runtime_error("Fila de ubicacion invalida.");
            }

            if (location.id < 0 || location.id >= locationCount) {
                throw runtime_error("ID de ubicacion fuera de rango.");
            }

            if (!isfinite(location.x) || !isfinite(location.y)) {
                throw runtime_error(
                    "Las coordenadas de una ubicacion no son finitas."
                );
            }

            if (location.ready > location.due) {
                throw runtime_error("Ventana temporal invalida.");
            }

            if (location.serviceTime < 0) {
                throw runtime_error(
                    "Tiempo de servicio de ubicacion negativo."
                );
            }

            if (location.type < 0 ||
                location.type >= instance.typeCount) {
                throw runtime_error("Tipo de ubicacion fuera de rango.");
            }

            if (location.type == 1 && location.capacity < 0) {
                throw runtime_error(
                    "Una ubicacion compartida debe tener capacidad no negativa."
                );
            }

            // Ajustar DUE según el tipo de ubicación y escalar
            // ventanas temporales y tiempo de servicio por factor 10.
            if (location.type == 2) {
                location.due += 5;  // Ubicación individual.
            } else if (location.type == 1) {
                location.due += 2;  // Ubicación compartida (SDL).
            }

            location.ready *= 10;
            location.due *= 10;
            location.serviceTime *= 10;

            instance.locations[location.id] = location;
        }

        if (instance.locations[0].id != 0 ||
            instance.locations[0].type != 0) {
            throw runtime_error(
                "La ubicacion 0 debe corresponder al deposito (tipo 0)."
            );
        }


        skipSectionHeader(file);

        for (int i = 0; i < optionCount; ++i) {
            if (!nextLine(file, line)) {
                throw runtime_error("Faltan datos de opciones.");
            }

            prepareLine(stream, line);

            Option option{};

            if (!(stream >> option.id
                         >> option.locationId
                         >> option.customerId
                         >> option.priority
                         >> option.serviceTime
                         >> option.cost)) {
                throw runtime_error("Fila de opcion invalida.");
            }

            if (option.id < 0 || option.id >= optionCount) {
                throw runtime_error("ID de opcion fuera de rango.");
            }

            if (option.locationId < 0 ||
                option.locationId >= locationCount) {
                throw runtime_error("Ubicacion de opcion inexistente.");
            }

            if (option.customerId < 0 ||
                option.customerId >= customerCount) {
                throw runtime_error("Cliente de opcion inexistente.");
            }

            if (option.priority < 0 ||
                option.priority >= instance.priorityCount) {
                throw runtime_error("Prioridad fuera de rango.");
            }

            if (option.serviceTime < 0) {
                throw runtime_error("Tiempo de servicio negativo.");
            }

            // Escalar también el tiempo de servicio de cada opción.
            option.serviceTime *= 10;

            instance.options[option.id] = option;

            // Construir relaciones auxiliares.
            instance.optionsByCustomer[option.customerId]
                .push_back(option.id);

            instance.optionsByLocation[option.locationId]
                .push_back(option.id);
        }


        for (int customerId = 0;
             customerId < customerCount;
             ++customerId) {

            if (instance.optionsByCustomer[customerId].empty()) {
                throw runtime_error(
                    "El cliente " + to_string(customerId) +
                    " no tiene opciones."
                );
            }
        }

        //cout << "Instancia leida correctamente: "<< filename << endl;

        return true;
    }
    catch (const exception& error) {
        // No dejar una instancia parcialmente cargada ante un error.
        instance = Instance{};

        cerr << "Error al leer instancia: "
             << error.what() << endl;

        return false;
    }
}


#include "../include/parser.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;

// Obtiene la siguiente linea no vacia.
static bool nextLine(ifstream& file, string& line) {
    while (getline(file, line)) {
        if (line.find_first_not_of(" \t\r\n") != string::npos) {
            return true;
        }
    }

    return false;
}

// Lee una seccion compuesta por una cabecera y sus datos.
static void skipSectionHeader(ifstream& file) {
    string line;

    if (!nextLine(file, line)) {
        throw runtime_error("Falta una cabecera de seccion.");
    }
}

bool readInstance(const string& filename, Instance& instance) {
    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "Error: no se pudo abrir " << filename << endl;
        return false;
    }

    try {
        string line;
        istringstream stream;

        // --------------------------------
        // 1. Parametros generales
        // --------------------------------

        // Saltar encabezado descriptivo
        if (!nextLine(file, line)) {
            throw runtime_error("Archivo vacio.");
        }

        // Saltar encabezado de parametros generales
        if (!nextLine(file, line)) {
            throw runtime_error("Falta el encabezado de parametros generales.");
        }

        // Leer valores generales
        if (!nextLine(file, line)) {
            throw runtime_error("Faltan los parametros generales.");
        }

        stream.clear();
        stream.str(line);

        int locationCount;
        int customerCount;
        int optionCount;

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
            optionCount <= 0) {
            throw runtime_error("Las dimensiones deben ser positivas.");
        }

        // Preparar vectores
        instance.customers.resize(customerCount);
        instance.locations.resize(locationCount);
        instance.options.resize(optionCount);

        instance.optionsByCustomer.assign(
            customerCount, vector<int>()
        );

        instance.optionsByLocation.assign(
            locationCount, vector<int>()
        );

        // --------------------------------
        // 2. Clientes
        // --------------------------------

        skipSectionHeader(file);

        for (int i = 0; i < customerCount; i++) {
            if (!nextLine(file, line)) {
                throw runtime_error("Faltan datos de clientes.");
            }

            stream.clear();
            stream.str(line);

            Customer customer;

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

        // --------------------------------
        // 3. Ubicaciones
        // --------------------------------

        skipSectionHeader(file);

        for (int i = 0; i < locationCount; i++) {
            if (!nextLine(file, line)) {
                throw runtime_error("Faltan datos de ubicaciones.");
            }

            stream.clear();
            stream.str(line);

            Location location;

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

            if (location.ready > location.due) {
                throw runtime_error("Ventana temporal invalida.");
            }

            instance.locations[location.id] = location;
        }

        // --------------------------------
        // 4. Opciones
        // --------------------------------

        skipSectionHeader(file);

        for (int i = 0; i < optionCount; i++) {
            if (!nextLine(file, line)) {
                throw runtime_error("Faltan datos de opciones.");
            }

            stream.clear();
            stream.str(line);

            Option option;

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

            instance.options[option.id] = option;

            // Construir relaciones auxiliares
            instance.optionsByCustomer[option.customerId]
                .push_back(option.id);

            instance.optionsByLocation[option.locationId]
                .push_back(option.id);
        }

        // --------------------------------
        // 5. Validaciones finales
        // --------------------------------

        for (int i = 0; i < customerCount; i++) {
            if (instance.optionsByCustomer[i].empty()) {
                throw runtime_error(
                    "El cliente " + to_string(i) +
                    " no tiene opciones."
                );
            }
        }

        cout << "Instancia leida correctamente: "
             << filename << endl;

        return true;
    }
    catch (const exception& error) {
        cerr << "Error al leer instancia: "
             << error.what() << endl;

        return false;
    }
}

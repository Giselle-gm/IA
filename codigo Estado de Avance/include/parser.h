#ifndef PARSER_H
#define PARSER_H

#include <string>

#include "instance.h"

// Lee una instancia desde archivo y construye las tablas auxiliares.
// Retorna false si el archivo no puede abrirse o contiene datos invalidos.
bool readInstance(const std::string& filename, Instance& instance);

#endif

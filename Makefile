
# =====================================
# Configuracion del compilador
# =====================================

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
CPPFLAGS = -Iinclude

# =====================================
# Directorios
# =====================================

SRC_DIR = src
INC_DIR = include
BIN_DIR = bin

# =====================================
# Ejecutable
# =====================================

TARGET = $(BIN_DIR)/vrpdo

# =====================================
# Archivos fuente
# =====================================

SRCS = main.cpp \
       $(SRC_DIR)/parser.cpp \
       $(SRC_DIR)/evaluator.cpp \
       $(SRC_DIR)/greedy.cpp \
       $(SRC_DIR)/simulated_annealing.cpp

# Archivos objeto
OBJS = $(patsubst %.cpp,$(BIN_DIR)/%.o,$(SRCS))

# Dependencias automaticas de headers
DEPS = $(OBJS:.o=.d)

# =====================================
# Compilacion
# =====================================

all: $(TARGET)

$(TARGET): $(OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

# =====================================
# Ejecucion
# =====================================

run: $(TARGET)
	./$(TARGET) instances/25/U_25large_1.txt

# Permite seleccionar una instancia:
# make run INSTANCE=instances/50/V_50large_1.txt

INSTANCE ?= instances/25/U_25large_1.txt

run-instance: $(TARGET)
	./$(TARGET) $(INSTANCE)

# =====================================
# Limpieza
# =====================================

clean:
	rm -rf $(BIN_DIR)

-include $(DEPS)

.PHONY: all run run-instance clean

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

INSTANCE ?= instances/25/U_25large_1.txt

run-instance: $(TARGET)
	./$(TARGET) $(INSTANCE)

# =====================================
# Prueba de factibilidad de todas las instancias
# =====================================

TEST_TARGET = $(BIN_DIR)/test_instances
TEST_SRCS = test_instances.cpp
TEST_OBJS = $(patsubst %.cpp,$(BIN_DIR)/%.o,$(TEST_SRCS)) \
            $(BIN_DIR)/$(SRC_DIR)/parser.o \
            $(BIN_DIR)/$(SRC_DIR)/evaluator.o \
            $(BIN_DIR)/$(SRC_DIR)/greedy.o

$(TEST_TARGET): $(TEST_OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

test-instances: $(TEST_TARGET)
	./$(TEST_TARGET)

# =====================================
# Evaluacion de Simulated Annealing en todas las instancias
# =====================================

TEST_SA_TARGET = $(BIN_DIR)/test_sa
TEST_SA_SRCS = test_sa.cpp
TEST_SA_OBJS = $(patsubst %.cpp,$(BIN_DIR)/%.o,$(TEST_SA_SRCS)) \
               $(BIN_DIR)/$(SRC_DIR)/parser.o \
               $(BIN_DIR)/$(SRC_DIR)/evaluator.o \
               $(BIN_DIR)/$(SRC_DIR)/greedy.o \
               $(BIN_DIR)/$(SRC_DIR)/simulated_annealing.o

$(TEST_SA_TARGET): $(TEST_SA_OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

test-sa: $(TEST_SA_TARGET)
	./$(TEST_SA_TARGET)

# =====================================
# Limpieza
# =====================================

clean:
	rm -rf $(BIN_DIR)

-include $(DEPS)

.PHONY: all run run-instance test-instances test-sa clean

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
# Diagnostico de instancias no resueltas por Greedy
# =====================================

TEST_GREEDY_FAILURES_TARGET = $(BIN_DIR)/test_greedy_failures
TEST_GREEDY_FAILURES_SRCS = test_greedy_failures.cpp
TEST_GREEDY_FAILURES_OBJS = $(patsubst %.cpp,$(BIN_DIR)/%.o,$(TEST_GREEDY_FAILURES_SRCS)) \
							$(BIN_DIR)/$(SRC_DIR)/parser.o \
							$(BIN_DIR)/$(SRC_DIR)/evaluator.o \
							$(BIN_DIR)/$(SRC_DIR)/greedy.o

$(TEST_GREEDY_FAILURES_TARGET): $(TEST_GREEDY_FAILURES_OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

test-greedy-failures: $(TEST_GREEDY_FAILURES_TARGET)
	./$(TEST_GREEDY_FAILURES_TARGET)

# =====================================
# Experimento con limites crecientes de retroceso Greedy
# =====================================

TEST_GREEDY_LIMITS_TARGET = $(BIN_DIR)/test_greedy_limits
TEST_GREEDY_LIMITS_SRCS = test_greedy_limits.cpp
TEST_GREEDY_LIMITS_OBJS = $(patsubst %.cpp,$(BIN_DIR)/%.o,$(TEST_GREEDY_LIMITS_SRCS)) \
						  $(BIN_DIR)/$(SRC_DIR)/parser.o \
						  $(BIN_DIR)/$(SRC_DIR)/evaluator.o \
						  $(BIN_DIR)/$(SRC_DIR)/greedy.o

$(TEST_GREEDY_LIMITS_TARGET): $(TEST_GREEDY_LIMITS_OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

test-greedy-limits: $(TEST_GREEDY_LIMITS_TARGET)
	./$(TEST_GREEDY_LIMITS_TARGET)


# =====================================
# Caracterizacion de instancias restantes de Greedy
# =====================================

TEST_GREEDY_REMAINING_TARGET = $(BIN_DIR)/test_greedy_remaining
TEST_GREEDY_REMAINING_SRCS = test_greedy_remaining.cpp
TEST_GREEDY_REMAINING_OBJS = $(patsubst %.cpp,$(BIN_DIR)/%.o,$(TEST_GREEDY_REMAINING_SRCS)) \
							 $(BIN_DIR)/$(SRC_DIR)/parser.o \
							 $(BIN_DIR)/$(SRC_DIR)/evaluator.o \
							 $(BIN_DIR)/$(SRC_DIR)/greedy.o

$(TEST_GREEDY_REMAINING_TARGET): $(TEST_GREEDY_REMAINING_OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

test-greedy-remaining: $(TEST_GREEDY_REMAINING_TARGET)
	./$(TEST_GREEDY_REMAINING_TARGET)


# =====================================
# Comparacion de prioridades de seleccion Greedy
# =====================================

TEST_GREEDY_PRIORITIES_TARGET = $(BIN_DIR)/test_greedy_priorities
TEST_GREEDY_PRIORITIES_SRCS = test_greedy_priorities.cpp
TEST_GREEDY_PRIORITIES_OBJS = $(patsubst %.cpp,$(BIN_DIR)/%.o,$(TEST_GREEDY_PRIORITIES_SRCS)) \
							  $(BIN_DIR)/$(SRC_DIR)/parser.o \
							  $(BIN_DIR)/$(SRC_DIR)/evaluator.o \
							  $(BIN_DIR)/$(SRC_DIR)/greedy.o

$(TEST_GREEDY_PRIORITIES_TARGET): $(TEST_GREEDY_PRIORITIES_OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

test-greedy-priorities: $(TEST_GREEDY_PRIORITIES_TARGET)
	./$(TEST_GREEDY_PRIORITIES_TARGET)


TEST_GREEDY_COMPARISON_TARGET = $(BIN_DIR)/test_greedy_comparison
TEST_GREEDY_COMPARISON_SRCS = test_greedy_comparison.cpp
TEST_GREEDY_COMPARISON_OBJS = $(patsubst %.cpp,$(BIN_DIR)/%.o,$(TEST_GREEDY_COMPARISON_SRCS)) \\
							$(BIN_DIR)/$(SRC_DIR)/parser.o \\
							$(BIN_DIR)/$(SRC_DIR)/evaluator.o \\
							$(BIN_DIR)/$(SRC_DIR)/greedy.o

$(TEST_GREEDY_COMPARISON_TARGET): $(TEST_GREEDY_COMPARISON_OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

test-greedy-comparison: $(TEST_GREEDY_COMPARISON_TARGET)
	./$(TEST_GREEDY_COMPARISON_TARGET)

# =====================================
# Limpieza
# =====================================

clean:
	rm -rf $(BIN_DIR)

-include $(DEPS)

.PHONY: all run run-instance test-instances test-sa test-greedy-failures test-greedy-limits test-greedy-remaining test-greedy-priorities test-greedy-comparison clean

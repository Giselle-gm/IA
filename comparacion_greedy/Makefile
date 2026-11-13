CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
CPPFLAGS = -Iinclude

SRC_DIR = src
BIN_DIR = bin

TARGET = $(BIN_DIR)/vrpdo

SRCS = main.cpp $(SRC_DIR)/parser.cpp $(SRC_DIR)/evaluador.cpp $(SRC_DIR)/greedy.cpp $(SRC_DIR)/simulated_annealing.cpp
OBJS = $(patsubst %.cpp,$(BIN_DIR)/%.o,$(SRCS))

DEPS = $(OBJS:.o=.d)


TEST_VARIANTS = $(BIN_DIR)/test_greedy_variants
TEST_VARIANTS_OBJS = $(BIN_DIR)/test_greedy_variants.o $(BIN_DIR)/$(SRC_DIR)/parser.o $(BIN_DIR)/$(SRC_DIR)/evaluador.o $(BIN_DIR)/$(SRC_DIR)/greedy_variants.o

all: $(TARGET)
$(TARGET): $(OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@
$(BIN_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

test-greedy-variants: $(TEST_VARIANTS)
	./$(TEST_VARIANTS)
$(TEST_VARIANTS): $(TEST_VARIANTS_OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

run: $(TARGET)
	./$(TARGET) instances/25/U_25large_1.txt
INSTANCE ?= instances/25/U_25large_1.txt

run-instance: $(TARGET)
	./$(TARGET) $(INSTANCE)

clean:
	rm -rf $(BIN_DIR) greedy_backtracking_comparison.csv
-include $(DEPS) $(TEST_VARIANTS_OBJS:.o=.d)

.PHONY: all run run-instance test-greedy-variants clean

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
CPPFLAGS = -Iinclude

SRC_DIR = src
BIN_DIR = bin

TARGET = $(BIN_DIR)/vrpdo

SRCS = main.cpp \
        $(SRC_DIR)/parser.cpp \
        $(SRC_DIR)/evaluator.cpp \
        $(SRC_DIR)/greedy.cpp \
        $(SRC_DIR)/simulated_annealing.cpp

OBJS = $(patsubst %.cpp,$(BIN_DIR)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	./$(TARGET) instances/25/U_25large_1.txt

INSTANCE ?= instances/25/U_25large_1.txt

run-instance: $(TARGET)
	./$(TARGET) $(INSTANCE)

TEST_TARGET = $(BIN_DIR)/test_instances
TEST_OBJS = $(BIN_DIR)/test_instances.o \
            $(BIN_DIR)/$(SRC_DIR)/parser.o \
            $(BIN_DIR)/$(SRC_DIR)/evaluator.o \
            $(BIN_DIR)/$(SRC_DIR)/greedy.o

$(TEST_TARGET): $(TEST_OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

test-instances: $(TEST_TARGET)
	./$(TEST_TARGET)

TEST_SA_TARGET = $(BIN_DIR)/test_sa
TEST_SA_OBJS = $(BIN_DIR)/test_sa.o \
               $(BIN_DIR)/$(SRC_DIR)/parser.o \
               $(BIN_DIR)/$(SRC_DIR)/evaluator.o \
               $(BIN_DIR)/$(SRC_DIR)/greedy.o \
               $(BIN_DIR)/$(SRC_DIR)/simulated_annealing.o

$(TEST_SA_TARGET): $(TEST_SA_OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

test-sa: $(TEST_SA_TARGET)
	./$(TEST_SA_TARGET)

TEST_GREEDY_TARGET = $(BIN_DIR)/test_greedy
TEST_GREEDY_OBJS = $(BIN_DIR)/test_greedy.o \
                   $(BIN_DIR)/$(SRC_DIR)/parser.o \
                   $(BIN_DIR)/$(SRC_DIR)/evaluator.o \
                   $(BIN_DIR)/$(SRC_DIR)/greedy.o

$(TEST_GREEDY_TARGET): $(TEST_GREEDY_OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

test-greedy: $(TEST_GREEDY_TARGET)
	./$(TEST_GREEDY_TARGET)

clean:
	rm -rf $(BIN_DIR)

-include $(DEPS)

.PHONY: all run run-instance test-instances test-sa test-greedy clean

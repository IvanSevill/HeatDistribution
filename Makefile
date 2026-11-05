# Headless build for Linux and other GCC/Clang systems. The window needs Raylib and is
# built on Windows from HeatSimulation_Parallel.sln.
CC ?= gcc
CXX ?= g++
GRID_SIZE ?= 48
BUILD_DIR ?= build
FLAGS = -O2 -Wall -Wextra -fopenmp -I. -DGRID_SIZE=$(GRID_SIZE) -DHEAT_NO_GUI

SOLVER_SOURCES = data.c logic.c jacobi.c gauss-seidel.c sor.c
SOLVER_OBJECTS = $(SOLVER_SOURCES:%.c=$(BUILD_DIR)/%.o)
HEADERS = definitions.h

.PHONY: all test benchmark clean

all: $(BUILD_DIR)/heat

$(BUILD_DIR)/%.o: %.c $(HEADERS) | $(BUILD_DIR)
	$(CC) $(FLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: main.cpp $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(FLAGS) -c $< -o $@

$(BUILD_DIR)/heat: $(BUILD_DIR)/main.o $(SOLVER_OBJECTS)
	$(CXX) $(FLAGS) $^ -lm -o $@

$(BUILD_DIR)/test_solvers: tests/test_solvers.c $(SOLVER_OBJECTS) $(HEADERS)
	$(CC) $(FLAGS) tests/test_solvers.c $(SOLVER_OBJECTS) -lm -o $@

$(BUILD_DIR):
	mkdir -p $@

test: $(BUILD_DIR)/heat $(BUILD_DIR)/test_solvers
	$(BUILD_DIR)/test_solvers
	bash tests/test_cli.sh $(BUILD_DIR)/heat

benchmark:
	bash scripts/benchmark.sh

clean:
	rm -rf $(BUILD_DIR)

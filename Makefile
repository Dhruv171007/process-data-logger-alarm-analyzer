# Process Data Logger and Alarm Analyzer - Makefile
# Targets: all, run, test, clean

# ---------- Compilers, options and libraries ----------
C_COMPILER     = gcc
CPP_COMPILER   = g++
C_OPTIONS      = -Wall -Wextra -Wpedantic -std=c11 -Iinclude -MMD -MP
CPP_OPTIONS    = -Wall -Wextra -Wpedantic -std=c++17 -Iinclude
APP_LIBRARIES  = -lm
TEST_LIBRARIES = -lgtest -lgtest_main -pthread -lm

# ---------- Folders and files ----------
BUILD    = build
TARGET   = $(BUILD)/analyzer.exe
TEST_BIN = $(BUILD)/run_tests.exe

SRC      = $(wildcard src/*.c)
OBJ      = $(patsubst src/%.c,$(BUILD)/%.o,$(SRC))
LIB_OBJ  = $(filter-out $(BUILD)/main.o,$(OBJ))
TEST_SRC = $(wildcard tests/*.cpp)

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(C_COMPILER) $(OBJ) -o $@ $(APP_LIBRARIES)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(C_COMPILER) $(C_OPTIONS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

run: $(TARGET)
	./$(TARGET)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(LIB_OBJ) $(TEST_SRC) | $(BUILD)
	$(CPP_COMPILER) $(CPP_OPTIONS) $(TEST_SRC) $(LIB_OBJ) -o $@ $(TEST_LIBRARIES)

clean:
	rm -rf $(BUILD)

-include $(wildcard $(BUILD)/*.d)
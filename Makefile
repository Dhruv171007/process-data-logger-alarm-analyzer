# Process Data Logger and Alarm Analyzer - Makefile
# Targets: all, run, test, clean

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
	gcc $(OBJ) -o $@ -lm

$(BUILD)/%.o: src/%.c | $(BUILD)
	gcc -Wall -Wextra -Wpedantic -std=c11 -Iinclude -MMD -MP -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

run: $(TARGET)
	./$(TARGET)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(LIB_OBJ) $(TEST_SRC) | $(BUILD)
	g++ -Wall -Wextra -Wpedantic -std=c++17 -Iinclude $(TEST_SRC) $(LIB_OBJ) -o $@ -lgtest -lgtest_main -pthread -lm

clean:
	rm -rf $(BUILD)

-include $(wildcard $(BUILD)/*.d)
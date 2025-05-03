# Compiler & flags
CXX := g++
ifeq ($(DEBUG),1)
    CXXFLAGS := -std=c++17 -Wall -Wextra -g -DDEBUG -Iinclude -MMD -MP
else
    CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -DNDEBUG -Iinclude -MMD -MP
endif
LDFLAGS := -lncurses

SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin

LIBS := chess engine ui

chess_SRC := $(wildcard $(SRC_DIR)/chess/*.cpp)
engine_SRC := $(wildcard $(SRC_DIR)/engine/*.cpp)
ui_SRC := $(wildcard $(SRC_DIR)/ui/*.cpp)

chess_OBJ := $(chess_SRC:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
engine_OBJ := $(engine_SRC:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
ui_OBJ := $(ui_SRC:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

MAIN_SRC := $(SRC_DIR)/main.cpp
MAIN_OBJ := $(BUILD_DIR)/main.o

ALL_OBJ := $(chess_OBJ) $(engine_OBJ) $(ui_OBJ) $(MAIN_OBJ)

TARGET := $(BIN_DIR)/chess-engine

all: $(TARGET)

# Main linking
$(TARGET): $(ALL_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Generic compilation rule for all .cpp files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile main separately (redundant due to the generic rule, kept for clarity)
$(MAIN_OBJ): $(MAIN_SRC)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Zobrist hash table dependency
src/chess/zobrist.hpp: extra/zobrist_generator.cpp
	@$(MAKE) gen_zobrist

# Magic bitboard dependencies
src/chess/magic.inc src/chess/magic_attacks.inc: extra/magic_generator.cpp
	@echo "Generating magic.inc and magic_attacks.inc..."
	@mkdir -p src/chess
	$(CXX) -std=c++17 -Wall -Wextra $< -o gen_magic.out
	./gen_magic.out src/chess/magic.inc src/chess/magic_attacks.inc
	@rm -f gen_magic.out

$(chess_OBJ): src/chess/magic.inc

# Selectively build libraries with: make library-select LIBS="chess engine"
library-select:
	@$(MAKE) selected-libs LIB_LIST="$(LIBS)"

selected-libs:
	@mkdir -p $(BUILD_DIR)/$(LIB_LIST)
	$(foreach lib,$(LIB_LIST), \
	  $(MAKE) $(BUILD_DIR)/$(lib);)

# Zobrist generator target
gen_zobrist: extra/zobrist_generator.cpp
	@echo "Generating Zobrist hash table..."
	@mkdir -p src/chess
	$(CXX) -std=c++17 -Wall -Wextra $< -o gen_zobrist.out
	./gen_zobrist.out > src/chess/zobrist.hpp
	@rm -f gen_zobrist.out


# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# TAR packaging
tar: clean
	@echo "Packaging project..."
	@if [ -z "$(TAR_OUTPUT)" ]; then \
	  echo "Error: TAR_OUTPUT variable not set. Usage: make tar TAR_OUTPUT=name"; \
	  exit 1; \
	fi
	@mkdir -p $(TAR_OUTPUT)
	@if [ -f .tarinclude ]; then \
	  echo "Using .tarinclude..."; \
	  while IFS= read -r line || [ -n "$$line" ]; do \
	    [ -z "$$line" ] && continue; \
	    cp -r --parents "$$line" $(TAR_OUTPUT)/; \
	  done < .tarinclude; \
	else \
	  echo "No .tarinclude found; copying entire project (except artifacts)..."; \
	  rsync -a --exclude="$(TAR_OUTPUT)" --exclude="$(TAR_OUTPUT).tar.gz" --exclude='.git' ./ $(TAR_OUTPUT)/; \
	fi
	@tar -czf $(TAR_OUTPUT).tar.gz $(TAR_OUTPUT)
	@rm -rf $(TAR_OUTPUT)
	@echo "Created archive: $(TAR_OUTPUT).tar.gz"

# Automatically include generated dependency files
-include $(ALL_OBJ:.o=.d)

.PHONY: all clean library-select selected-libs

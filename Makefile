CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread -Iinclude
OBJ_DIR = obj
BIN_DIR = bin

_OBJ_MASTER = master.o
_OBJ_SLAVE = slave.o
OBJ_MASTER = $(addprefix $(OBJ_DIR)/, $(_OBJ_MASTER))
OBJ_SLAVE = $(addprefix $(OBJ_DIR)/, $(_OBJ_SLAVE))

all: master slave

master: $(OBJ_MASTER) | $(BIN_DIR)
	$(CXX) $^ -o $(BIN_DIR)/$@

slave: $(OBJ_SLAVE) | $(BIN_DIR)
	$(CXX) $^ -o $(BIN_DIR)/$@

$(OBJ_DIR)/%.o: src/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	rm -f $(OBJ_DIR)/*.o $(BIN_DIR)/master $(BIN_DIR)/slave

.PHONY: all master slave clean

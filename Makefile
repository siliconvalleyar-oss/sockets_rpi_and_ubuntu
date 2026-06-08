CXX       = g++
CXXFLAGS  = -std=c++17 -Wall -Wextra -O2 -pthread -Iinclude
LDFLAGS   = -pthread

OBJ_DIR   = obj
BIN_DIR   = bin

_OBJS     = master.o slave.o
OBJS      = $(addprefix $(OBJ_DIR)/, $(_OBJS))
TARGETS   = master slave

# --- Cross-compile for Raspberry Pi (ARM) ---
#   make CROSS_COMPILE=arm-linux-gnueabihf- all
ifdef CROSS_COMPILE
  CXX   := $(CROSS_COMPILE)$(CXX)
  SYSROOT := $(CROSS_COMPILE)gcc --print-sysroot 2>/dev/null
endif

# --- Installation paths ---
PREFIX    ?= /usr/local
BINDIR    ?= $(PREFIX)/bin

.PHONY: all clean install uninstall dist

all: $(TARGETS)

master: $(OBJ_DIR)/master.o | $(BIN_DIR)
	$(CXX) $^ -o $(BIN_DIR)/$@ $(LDFLAGS)

slave: $(OBJ_DIR)/slave.o | $(BIN_DIR)
	$(CXX) $^ -o $(BIN_DIR)/$@ $(LDFLAGS)

$(OBJ_DIR)/%.o: src/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(BIN_DIR)/master $(DESTDIR)$(BINDIR)/
	install -m 755 $(BIN_DIR)/slave  $(DESTDIR)$(BINDIR)/

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/master $(DESTDIR)$(BINDIR)/slave

dist:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	tar czf sockets-project.tar.gz \
		--transform='s,^,sockets/,' \
		src/ include/ Makefile README.md build_sockets.sh

# Targets:
#   make            – build the 'vpsm' binary (default)
#   make debug      – build with AddressSanitizer + debug symbols
#   make clean      – remove build artefacts
#   make install    – install to $(PREFIX)/bin  (default: /usr/local/bin)
#   make uninstall  – remove the installed binary
#
# Variables you can override on the command line:
#   CC        C compiler  (default: cc)
#   PREFIX    install root (default: /usr/local)
#   DESTDIR   staging root for package builds
#
# Example:
#   make CC=clang PREFIX=$HOME/.local install

CC      ?= cc
PREFIX  ?= /usr/local
DESTDIR ?=

TARGET  := vpsm
SRCS    := main.c server.c store.c ssh.c util.c
OBJS    := $(SRCS:.c=.o)

# Strict warnings; only standard C99; no external libraries required
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic \
           -Wstrict-prototypes -Wmissing-prototypes \
           -Wno-unused-parameter

# Release flags
RFLAGS  := -O2 -DNDEBUG

# Debug flags
DFLAGS  := -O0 -g3 -fsanitize=address,undefined

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(RFLAGS) -o $@ $^
	@echo "Built: $(TARGET)"

%.o: %.c vpsm.h
	$(CC) $(CFLAGS) $(RFLAGS) -c -o $@ $<


.PHONY: debug
debug: CFLAGS += $(DFLAGS)
debug: RFLAGS  =
debug: $(TARGET)


INSTALL_DIR := $(DESTDIR)$(PREFIX)/bin

.PHONY: install
install: all
	install -d $(INSTALL_DIR)
	install -m 755 $(TARGET) $(INSTALL_DIR)/$(TARGET)
	@echo "Installed: $(INSTALL_DIR)/$(TARGET)"

.PHONY: uninstall
uninstall:
	rm -f $(INSTALL_DIR)/$(TARGET)
	@echo "Removed:   $(INSTALL_DIR)/$(TARGET)"


.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Cleaned."

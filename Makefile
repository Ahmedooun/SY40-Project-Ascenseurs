## Makefile

```makefile
CC       := gcc
CFLAGS   := -std=c11 -Wall -Wextra -Werror -pthread -Iinclude
LDFLAGS  := -pthread

SRC_DIR  := src
OBJ_DIR  := build
BIN      := sim

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all run clean distclean

all: $(BIN)

# Crée le binaire
$(BIN): $(OBJ_DIR) ipc.key $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compile chaque .c en .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Crée le dossier build si nécessaire
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Assure l’existence du fichier utilisé par ftok()
ipc.key:
	touch ipc.key

run: all
	./$(BIN)

clean:
	rm -rf $(OBJ_DIR)

distclean: clean
	rm -f $(BIN) stats.csv


CC      = gcc
#CFLAGS  = -O3 -lm -D_FILE_OFFSET_BITS=64
CFLAGS = -O1 -lm -D_FILE_OFFSET_BITS=64
SRC_DIR = src
BIN_DIR = bin

# Detect platform
ifeq ($(OS), Windows_NT)
    EXT     = .exe
    MKDIR   = if not exist $(BIN_DIR) mkdir $(BIN_DIR)
    CLEAN   = del /Q $(BIN_DIR)\*
    DEVNULL = nul
    TOUCH   = type nul >
else
    EXT     = .out
    MKDIR   = mkdir -p $(BIN_DIR)
    CLEAN   = rm -f $(BIN_DIR)/*
    DEVNULL = /dev/null
    TOUCH   = touch
endif

## added pulsar_det_an_v4 to compile - remove after testing, 20260416, Marko Radolovic
TARGETS = $(BIN_DIR)/FileTrim$(EXT) \
          $(BIN_DIR)/RAFFT22Lg$(EXT) \
          $(BIN_DIR)/RTLChannel4bin$(EXT) \
          $(BIN_DIR)/pulsar_det_an_v4$(EXT) \
          $(BIN_DIR)/rapulsar2con$(EXT) \
          $(BIN_DIR)/Blanks.txt \
          $(BIN_DIR)/Blankf.txt \

.PHONY: all clean

all: $(BIN_DIR) $(TARGETS)

$(BIN_DIR):
	$(MKDIR)

$(BIN_DIR)/%$(EXT): $(SRC_DIR)/%.c
	$(CC) $< -o $@ $(CFLAGS)

$(BIN_DIR)/Blanks.txt $(BIN_DIR)/Blankf.txt: | $(BIN_DIR)
	$(TOUCH) $@

clean:
	$(CLEAN)

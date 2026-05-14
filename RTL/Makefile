CC      = gcc
#CFLAGS  = -O3 -lm -D_FILE_OFFSET_BITS=64
CFLAGS = -O0  -g3 -D_FILE_OFFSET_BITS=64 -Wall -Wpedantic -Wextra   -fno-omit-frame-pointer -fopt-info-vec-missed=vec.missed -march=native
LDFLAGS = -lm 
SRC_DIR = src
BIN_DIR = bin
INCLUDE_DIR = includes

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
          $(BIN_DIR)/Blankf.txt 



####################################################################################################
## object files from the numerics directory
$(BIN_DIR)/four.o : $(SRC_DIR)/numerics/four.c $(INCLUDE_DIR)/numerics/four.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/conv.o : $(SRC_DIR)/numerics/conv.c $(INCLUDE_DIR)/numerics/conv.h $(INCLUDE_DIR)/numerics/four.h 
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/psnr.o : $(SRC_DIR)/numerics/psnr.c $(INCLUDE_DIR)/numerics/psnr.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/DMSch.o : $(SRC_DIR)/numerics/DMSch.c $(INCLUDE_DIR)/numerics/DMSch.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/is_pow_of_2.o : $(SRC_DIR)/numerics/is_pow_of_2.c $(INCLUDE_DIR)/numerics/is_pow_of_2.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/pdSch.o : $(SRC_DIR)/numerics/pdSch.c $(INCLUDE_DIR)/numerics/pdSch.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/perSch.o : $(SRC_DIR)/numerics/perSch.c $(INCLUDE_DIR)/numerics/perSch.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/spectrum.o : $(SRC_DIR)/numerics/spectrum.c $(INCLUDE_DIR)/numerics/spectrum.h $(INCLUDE_DIR)/numerics/four.h 
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/targgaus.o : $(SRC_DIR)/numerics/targgaus.c $(INCLUDE_DIR)/numerics/targgaus.h $(INCLUDE_DIR)/numerics/four.h 
	$(CC) $(CFLAGS) -c $< -o $@

####################################################################################################
##object files from io directory
$(BIN_DIR)/files.o : $(SRC_DIR)/io/files.c
	$(CC) $(CFLAGS) -c $< -o $@
####################################################################################################
## object files that represent the targets
NUMERICS = \
	$(BIN_DIR)/four.o \
	$(BIN_DIR)/conv.o \
	$(BIN_DIR)/psnr.o \
	$(BIN_DIR)/DMSch.o \
	$(BIN_DIR)/is_pow_of_2.o \
	$(BIN_DIR)/pdSch.o \
	$(BIN_DIR)/perSch.o \
	$(BIN_DIR)/spectrum.o \
	$(BIN_DIR)/targgaus.o

IO = \
	$(BIN_DIR)/files.o

$(BIN_DIR)/pulsar_det_an_v4.o : $(SRC_DIR)/pulsar_det_an_v4.c $(NUMERICS)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/FileTrim$(EXT): $(SRC_DIR)/FileTrim.c 
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BIN_DIR)/RAFFT22Lg$(EXT): $(SRC_DIR)/RAFFT22Lg.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BIN_DIR)/rapulsar2con$(EXT): $(SRC_DIR)/rapulsar2con.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BIN_DIR)/RTLChannel4bin$(EXT): $(SRC_DIR)/RTLChannel4bin.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)
####################################################################################################
## Finally we link the o files for the relevant targets (right now only pulsar_det_an_v4)

$(BIN_DIR)/pulsar_det_an_v4$(EXT) : $(BIN_DIR)/pulsar_det_an_v4.o $(NUMERICS) $(IO)
	$(CC) $^ -o $@ $(LDFLAGS)
####################################################################################################
.PHONY: all clean

all: $(BIN_DIR) $(NUMERICS) $(IO) $(TARGETS)

$(BIN_DIR):
	$(MKDIR)


$(BIN_DIR)/Blanks.txt $(BIN_DIR)/Blankf.txt: | $(BIN_DIR)
	$(TOUCH) $@

clean:
	$(CLEAN)

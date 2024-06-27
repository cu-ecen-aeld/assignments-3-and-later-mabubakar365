# Default Compiler
CC = gcc
# Compiler flags
CFLAGS = -Wall -Werror -g -O0
# Linker flags
LDFLAGS = -lpthread
# Object files
OBJ=aesdsocket.o

# Check if CROSS_COMPILE is set
ifdef CROSS_COMPILE
	CC=$(CROSS_COMPILE)gcc
endif

# Default target
all: aesdsocket

aesdsocket: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

# Pattern rule for object files
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

# Clean target
clean:
	rm -f $(OBJ) aesdsocket

# Source Files
SRCS = aesdsocket.c

# Target Executable
TARGET = aesdsocket

.PHONY: all clean

# Default Compiler
CC = gcc
# Compiler flags
CFLAGS = -Wall -Werror -g -O0

# Object files
OBJ=aesdsocket.o

# Check if CROSS_COMPILE is set
ifdef CROSS_COMPILE
	CC=$(CROSS_COMPILE)gcc
endif

# Default target
all: aesdsocket

writer: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

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

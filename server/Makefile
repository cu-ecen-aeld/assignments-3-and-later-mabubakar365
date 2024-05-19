# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Werror -g -O0

# Source Files
SRCS = aesdsocket.c

# Target Executable
TARGET = aesdsocket

# Check if CROSS_COMPILE is set
ifdef CROSS_COMPILE
	CC=$(CROSS_COMPILE)gcc
endif

# Rule to build target
$(TARGET) : $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

# Clean Rule
clean:
	rm -f $(TARGET)

.DEFAULT_GOAL := all	

# Default Rule
all: $(TARGET)
# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Werror -g -O0

# Source Files
SRCS = aesdsocket.c

# Target Executable
TARGET = aesdsocket

# Rule to build target
$(TARGET) : $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

# Clean Rule
clean:
	rm -f $(TARGET)

.DEFAULT_GOAL := all	

# Default Rule
all: $(TARGET)
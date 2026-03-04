# Makefile for linked_list_sample application
# Simple standalone Makefile for demonstration

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -g -I../../src

# Source files
SRC = main.c \
	../../src/bacnet/basic/sys/linked_list.c

# Object files
OBJS = $(SRC:.c=.o)

# Target executable
TARGET = linked_list_sample

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@
	@echo "Build complete: $(TARGET)"

# Compile .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run the sample
run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -f $(TARGET) $(OBJS)
	rm -f ../../src/bacnet/basic/sys/linked_list.o

.PHONY: all clean run

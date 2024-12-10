# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -MMD

# Target executable
TARGET = sanju

# Source and object files
SRCS = sanju.c editorfunctions.c rawmode.c fileio.c viewerfunctions.c
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

# Rule to build the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Rule to compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Include dependency files
-include $(DEPS)

# Clean rule
clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

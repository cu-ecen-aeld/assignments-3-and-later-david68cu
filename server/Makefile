# Cross-compilation setup
# To compile for aarch64-linux-gnu-gcc, run:
# make CROSS_COMPILE=aarch64-linux-gnu-
ifdef CROSS_COMPILE
CC = $(CROSS_COMPILE)gcc
else
CC = gcc
endif

# Source files
SRCS = aesdsocket.c

# Object files
OBJS = $(SRCS:.c=.o)

# Target executable
TARGET = aesdsocket

# Default target
all: $(TARGET)

# Compile the target
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) -lm

# Clean target
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean



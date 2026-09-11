# Makefile for CS527 Mini-Computer System Simulator (Labs 1-4)

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
TARGET = simulator

# Source and Object files
SRCS = main.c compiler.c processor.c memory.c os.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET).exe *.byte simulation.log

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run

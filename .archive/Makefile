CC = gcc
CFLAGS = -Wall -Wextra -std=c11

TARGET = mini_computer

OBJS = main.o compiler.o processor.o memory.o

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

main.o: main.c compiler.h processor.h memory.h
	$(CC) $(CFLAGS) -c main.c

compiler.o: compiler.c compiler.h
	$(CC) $(CFLAGS) -c compiler.c

processor.o: processor.c processor.h memory.h
	$(CC) $(CFLAGS) -c processor.c

memory.o: memory.c memory.h
	$(CC) $(CFLAGS) -c memory.c

run:
	./mini_computer.exe 

clean:
	rm -f $(OBJS) $(TARGET)
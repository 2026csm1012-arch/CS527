CC = gcc
CFLAGS = -std=c99 -Wall -Wextra

simulator.exe: main.o compiler.o processor.o memory.o
	$(CC) main.o compiler.o processor.o memory.o -o simulator.exe

main.o: main.c compiler.h processor.h memory.h
	$(CC) $(CFLAGS) -c main.c

compiler.o: compiler.c compiler.h
	$(CC) $(CFLAGS) -c compiler.c

processor.o: processor.c processor.h memory.h
	$(CC) $(CFLAGS) -c processor.c

memory.o: memory.c memory.h
	$(CC) $(CFLAGS) -c memory.c

run: simulator.exe
	.\simulator.exe

clean:
	rm -f *.o simulator.exe

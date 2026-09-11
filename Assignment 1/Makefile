# Build the simulator

simulator.exe: main.o compiler.o processor.o memory.o
	gcc main.o compiler.o processor.o memory.o -o simulator.exe

# Compile main.c
main.o: main.c compiler.h processor.h memory.h
	gcc -c main.c

# Compile compiler.c
compiler.o: compiler.c compiler.h
	gcc -c compiler.c

# Compile processor.c
processor.o: processor.c processor.h memory.h
	gcc -c processor.c

# Compile memory.c
memory.o: memory.c memory.h
	gcc -c memory.c

# Run the simulator
run: simulator.exe
	.\simulator.exe

# Remove generated files
clean:
	del /Q *.o simulator.exe
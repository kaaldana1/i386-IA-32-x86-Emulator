CC=gcc
CFLAGS=-DDEBUG -g

x86emulator: program_loader.o gdt.o cpu.o decoder.o execute_stubs.o execute.o memory.o x86_emulator.o 
	$(CC) -o c.out program_loader.o gdt.o cpu.o decoder.o execute_stubs.o execute.o memory.o x86_emulator.o

clean:
	rm -f *.o *.out
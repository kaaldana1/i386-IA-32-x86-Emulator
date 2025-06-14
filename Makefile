CC=gcc
CFLAGS=-DDEBUG -g

x86emulator: decoder.o execute.o x86_emulator.o 
	$(CC) -o c.out decoder.o execute.o x86_emulator.o

clean:
	rm -f *.o *.out
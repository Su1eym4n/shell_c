CC=gcc
CFLAGS=-lreadline
newshell: shell.o
	$(CC) -o newshell shell.o $(CFLAGS)
clean:
	rm *.o newshell
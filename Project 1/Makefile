CC=g++
CFLAGS=-I.
DEPS = # header file 
OBJ = serverFork.o

%.o: %.c $(DEPS)
	$(CC) -cpp -o $@ $< $(CFLAGS)

serverFork: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)



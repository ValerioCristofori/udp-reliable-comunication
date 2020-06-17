CC = gcc
CFLAGS = -Wall
DEPS = defines.h
OBJ = client-udp.o utils.o

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

client: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

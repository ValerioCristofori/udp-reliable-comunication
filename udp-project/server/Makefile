CC = gcc
CFLAGS = -Wall -lpthread -pthread
DEPS = defines.h
OBJ = server-udp.o utils.o

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

server: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

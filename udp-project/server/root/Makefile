CC = gcc
CFLAGS = -Wall
DEPS = defines.h
OBJ = client-udp.o utils.o gbn-receiver.o gbn-sender.o

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

client-udp: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

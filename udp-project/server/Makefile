CC = gcc
CFLAGS = -Wall -lpthread -pthread
DEPS = defines.h
OBJ = utils.o server-udp.o gbn-sender.o gbn-receiver.o print.o datagram-setup.o

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

server-udp: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

CC = gcc
CFLAGS = -Wall -lm -DPUT_TEST
DEPS = defines.h
OBJ = client-udp.o utils.o gbn-receiver.o gbn-sender.o print.o datagram-setup.o

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

client-udp: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

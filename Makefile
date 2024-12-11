CC = gcc
CFLAGS = -Wall -std=c99 -D_POSIX_C_SOURCE=200809L -g -Wformat-truncation
LDFLAGS = -lrt -lpthread

TARGETS = parent child

all: $(TARGETS)

ipc_utils.o: ipc_utils.c ipc_utils.h
	$(CC) $(CFLAGS) -c ipc_utils.c

parent: parent.c ipc_utils.o
	$(CC) $(CFLAGS) -o $@ parent.c ipc_utils.o $(LDFLAGS)

child: child.c ipc_utils.o
	$(CC) $(CFLAGS) -o $@ child.c ipc_utils.o $(LDFLAGS)

clean:
	rm -f $(TARGETS) *.o

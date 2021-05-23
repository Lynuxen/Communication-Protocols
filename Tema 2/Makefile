# Protocoale de comunicatii:
# TCP/UDP
# Makefile

CFLAGS = -Wall -g 

PORT = 

IP_SERVER = 

all: server subscriber

# Compiles server.c
server: server.c -lm

# Compiles client.c
subscribe: subscriber.c

.PHONY: clean run_server run_client

# runs the server
run_server:
	./server ${PORT}

# runs the client
run_client:
	./subscriber ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber

CC=gcc
CFLAGS=-I.

tema3: tema3.c requests.c helpers.c buffer.c parson.c
	$(CC) -o tema3 tema3.c requests.c helpers.c buffer.c parson.c -g -Wall

run: tema3
	./tema3

clean:
	rm -f *.o tema3

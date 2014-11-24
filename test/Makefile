CC = gcc
PP = g++

all: server client

server: server.cpp
	$(PP) -o server server.cpp

client: client.c raw.c
	$(CC) client.c raw.c $(LOADLIBES) $(CFLAGS) -o client

clean:
	rm server client

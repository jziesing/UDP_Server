PP = g++
CC=g++
#CFLAGS=-Wall -W -g -Werror 
# Uncomment this line for the graduate student version
CFLAGS= -g  -DGRAD=1

LOADLIBES= -lnsl
all: server client

server: server.cpp
	$(PP) -o server server.cpp

client: client.c raw.c
	$(CC) client.c raw.c $(LOADLIBES) $(CFLAGS) -o client

clean:
	rm server client

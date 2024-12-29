
# E' possibile settare anche le seguenti flag:
# 	-DMDEBUG	MemoryDEBUG: stampa lo stato degli oggetti in sessione dopo ogni comando
# 	-DNDEBUG 	NetowrkDEBUG: stampa tutti i messaggi scambiati con send_msg e recv_msg
CFLAGS = -std=c89 -Wall -pedantic

all: server client

server: server.o lib/protocol.o lib/mystdlib.o lib/server/database.o lib/server/session.o lib/server/rooms.o
	gcc $(CFLAGS) server.o lib/protocol.o lib/mystdlib.o lib/server/database.o lib/server/session.o lib/server/rooms.o -o server

client: client.o lib/protocol.o lib/mystdlib.o
	gcc $(CFLAGS) client.o lib/protocol.o lib/mystdlib.o -o client

server.o: server.c
	gcc $(CFLAGS) -c server.c -o server.o

client.o: client.c
	gcc $(CFLAGS) -c client.c -o client.o

lib/protocol.o: lib/protocol.c
	gcc $(CFLAGS) -c lib/protocol.c -o lib/protocol.o

lib/mystdlib.o: lib/mystdlib.c
	gcc $(CFLAGS) -c lib/mystdlib.c -o lib/mystdlib.o

lib/server/database.o: lib/server/database.c
	gcc $(CFLAGS) -c lib/server/database.c -o lib/server/database.o

lib/server/session.o: lib/server/session.c
	gcc $(CFLAGS) -c lib/server/session.c -o lib/server/session.o

lib/server/rooms.o: lib/server/rooms.c
	gcc $(CFLAGS) -c lib/server/rooms.c -o lib/server/rooms.o

clean:
	rm -f *.o lib/*.o lib/server/*.o server client

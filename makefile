programa: client.o server.o
	@gcc client.o -o client
	@gcc server.o -o server

client.o: client.c
	@gcc -c client.c

server.o: server.c server.h
	@gcc -c server.c
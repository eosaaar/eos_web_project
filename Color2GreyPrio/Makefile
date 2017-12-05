all: client server

client: client.c 
	gcc client.c -o client -pthread

server: server.c
	gcc server.c -o server -pthread

csapp.o: csapp.c 
	gcc -c csapp.c -o csapp.o -pthread

clean:
	rm client server

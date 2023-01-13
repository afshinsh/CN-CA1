CC = g++ -std=c++11
CC1 = g++ -pthread -std=c++11

all : server client

server : main.o server.o
		$(CC1) main.o server.o -o server

client : client_main.o client.o 
		$(CC) client_main.o client.o -o client

client_main.o: client_main.cpp client.h
		$(CC) -c client_main.cpp -o client_main.o

main.o : main.cpp server.h
		$(CC) -c main.cpp -o main.o

server.o : server.cpp server.h message.h
		$(CC1) -c server.cpp -o server.o

client.o : client.cpp client.h server.h message.h
		$(CC) -c client.cpp -o client.o


.PHONY: clean
clean:
	rm -r *.o
	rm client
	rm server

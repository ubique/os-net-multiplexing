all: clean client server
		
client: client.cpp utils.h
		g++ -std=c++11 -pthread -Wall -o client.o client.cpp
		
server: server.cpp utils.h
		g++ -std=c++11 -pthread -Wall -o server.o server.cpp
		
runServer:
		./server.o 127.0.0.1 8080
		
runClient:
		./client.o 127.0.0.1 8080

clean:
		rm *.o

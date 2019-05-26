all: compile_server compile_client


compile_server: Server.o ./Server/server.cpp
	g++ Server.o ./Server/server.cpp -o server

Server.o: ./Server/Server.cpp
	g++ -c ./Server/Server.cpp -o Server.o


compile_client: Client.o ./Client/client.cpp
	g++ Client.o ./Client/client.cpp -o client

Client.o: ./Client/Client.cpp
	g++ -c ./Client/Client.cpp -o Client.o

clean:
	rm -v *.o ./server ./client


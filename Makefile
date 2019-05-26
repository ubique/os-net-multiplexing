all: client server

run: client server
	gnome-terminal -x sh -c "./server any 8080"
	gnome-terminal -x sh -c "./client localhost 8080"

client: client.o
	g++ -o client client.o

server: server.o
	g++ -o server server.o

client.o: client.cpp
	g++ -c client.cpp	

server.o: server.cpp
	g++ -c server.cpp

clean:
	rm -rf *.o *.so *.a main
	

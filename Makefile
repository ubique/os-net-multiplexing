all: client.cpp server.cpp
	g++ server.cpp -o server
	g++ client.cpp -o client


clean:
	rm -v ./server ./client

CXX = clang++
CXX_STANDARD = -std=c++17

all: buildAll

buildServer: server.cpp
	$(CXX) $(CXX_STANDARD) -o server server.cpp

buildClient: client.cpp
	$(CXX) $(CXX_STANDARD) -o client client.cpp

buildAll: buildServer buildClient

runServer: buildServer
	./server 127.0.0.1:12345

runClient: buildClient
	./client 127.0.0.1:12345

clean:
	rm -rf client server
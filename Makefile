all: server client
	

server: src/server.cpp 
	g++ -fsanitize=address,undefined $^ -o $@

client: src/client.cpp
	g++ -fsanitize=address,undefined $^ -o $@

clean: 
	rm -rf server client


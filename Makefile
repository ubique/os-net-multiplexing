all: server client
	

server: src/server.cpp src/rwutils.cpp src/rwutils.h 
	g++ -fsanitize=address,undefined $^ -o $@

client: src/client.cpp src/rwutils.cpp src/rwutils.h
	g++ -fsanitize=address,undefined $^ -o $@

clean: 
	rm -rf server client


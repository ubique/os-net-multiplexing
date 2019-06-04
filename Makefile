.PHONY: all clean

all:
	cc server.c -o server
	cc client.c -o client

clean:
	-rm server
	-rm client


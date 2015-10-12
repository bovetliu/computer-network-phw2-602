CC = gcc
DEBUG = -g
CFLAGS = -Wall -lpthread -c $(DEBUG)
LFLAGS = -Wall -lpthread $(DEBUG)

all: team8_client team8_server



clean:
	rm -rf *.o *~ team8_client team8_server


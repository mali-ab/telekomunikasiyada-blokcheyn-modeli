CC = g++

CFLAGS = -Wall -lssl -lcrypto

all: telekom_system

telekom_system: main.cpp blockchain.cpp signature.cpp smartContract.cpp
	$(CC) main.cpp blockchain.cpp signature.cpp smartContract.cpp -o telekom_system $(CFLAGS)

clean:
	rm -f telekom_system
	
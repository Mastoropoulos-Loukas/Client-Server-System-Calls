REQ = sniffer.o listener.o worker.o
CC = g++

dataServer: dataServer.cpp util.cpp
	$(CC) -o dataServer dataServer.cpp util.cpp -lpthread

remoteClient: remoteClient.cpp util.cpp
	$(CC) -o remoteClient remoteClient.cpp util.cpp

all: dataServer remoteClient

clean:
	rm -f dataServer remoteClient
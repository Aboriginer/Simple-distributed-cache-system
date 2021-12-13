CC = g++
CFLAGS = -std=c++11 -lpthread

all: master_main.cpp master.o ConsistentHash.o
	$(CC) $(CFLAGS) master_main.cpp  master.o ConsistentHash.o -o master_server -lpthread

ConsistentHash.o: ConsistentHash.cpp ConsistentHash.hpp 
	$(CC) $(CFLAGS) -c ConsistentHash.cpp -lpthread

master.o: master.cpp master.h
	$(CC) $(CFLAGS) -c master.cpp -lpthread

clean:
	rm -f *.o  master_server

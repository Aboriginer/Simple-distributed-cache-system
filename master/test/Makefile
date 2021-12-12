CC = g++
CFLAGS = -std=c++11

all: NewcliMain.cpp NewcacMain.cpp Newcli.o

	$(CC) $(CFLAGS) NewcliMain.cpp Newcli.o -o new_client
	$(CC) $(CFLAGS) NewcacMain.cpp Newcli.o -o new_cache

# new_ser.o: new_ser.cpp new_ser.h Common.h
# 	$(CC) $(CFLAGS) -c new_ser.cpp

Newcli.o: Newcli.cpp Newcli.h Common.h
	$(CC) $(CFLAGS) -c Newcli.cpp
	
clean:
	rm -f *.o new_client

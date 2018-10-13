CC = gcc
CFLAGS =
CFILES=$(shell ls *.c)
PROGS=$(CFILES:%.c=%)


install:server client
	
server:Server.c Persistence.c Persistence.h Utility.c Utility.h
	$(CC) $(CFLAGS) -pthread -o  $@ $^ -lrt  -DSQLITE_THREADSAFE=2 -l sqlite3

client:Client.c Persistence.c Persistence.h Utility.c Utility.h
	$(CC) $(CFLAGS) -pthread  -o  $@ $^ -lrt -DSQLITE_THREADSAFE=2 -l sqlite3

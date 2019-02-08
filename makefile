CC = gcc
CFLAGS = -Wextra -o3
CFILES=$(shell ls *.c)
PROGS=$(CFILES:%.c=%)

install: client server	

server:Server.c Persistence.c Persistence.h
	$(CC) $(CFLAGS) -o  $@ $^ -pthread -l sqlite3 

client:Client.c  Persistence.h
	$(CC) $(CFLAGS) -o  $@ $^

server_sql: Server.c Persistence.c Persistence.h ./sqlite/sqlite3.c ./sqlite/sqlite3.h 
	$(CC) -o3 -D SQLITE_FILE -o  $@ $^ -pthread -ldl

client_test: ./Test/Test_client.c 
	$(CC) $(CFLAGS) -o  $@ $^
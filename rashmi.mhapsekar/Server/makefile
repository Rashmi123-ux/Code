EXECNAME = tcp_server
CC=gcc
CFLAGS= -Wall

%.o : %.c 
	$(CC) -c -o $@ $< $(CFLAGS)
$(EXECNAME) : tcp_server.o
	$(CC) -o $(EXECNAME) tcp_server.o -pthread


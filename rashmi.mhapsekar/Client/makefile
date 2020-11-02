EXECNAME = tcp_client
CC=gcc
CFLAGS= -Wall

%.o : %.c 
	$(CC) -c -o $@ $< $(CFLAGS)
$(EXECNAME) : tcp_client.o
	$(CC) -o $(EXECNAME) tcp_client.o -pthread


PORT=56535
CFLAGS = -DPORT=$(PORT) -g -Wall -std=gnu99

all : hcq_server

hcq_server: hcq_server.o hcq.o
	gcc $(CFLAGS) -o hcq_server hcq_server.o hcq.o

hcq_server.o: hcq_server.c hcq_server.h
	gcc $(CFLAGS) -c hcq_server.c

clean: 
	rm helpcentre *.o

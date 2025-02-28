# Protocoale de comunicatii
# Laborator 7 - TCP
# Echo Server
# Makefile

CFLAGS = -Wall -g
LIBS = -lm

# Portul pe care asculta serverul
PORT = 12345

# Adresa IP a serverului
IP_SERVER = 127.0.0.1

all: server subscriber

common.o: common.c

# Compileaza server.c
server: server.c common.o $(LIBS)

# Compileaza subscriber.c
subscriber: subscriber.c common.o $(LIBS)

.PHONY: clean run_server run_client

# Ruleaza serverul
run_server:
	./server ${IP_SERVER} ${PORT}

# Ruleaza subscriber 	
run_subscriber:
	./subscriber "C1" ${IP_SERVER} ${PORT}

clean:
	rm -rf subscriber server *.o *.dSYM

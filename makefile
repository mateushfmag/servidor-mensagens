all:
	gcc -Wall -c ./common.c
	gcc -Wall ./client.c ./common.o -o client
	gcc -Wall ./server.c ./common.o -o server
clean:
	rm -rf client server client.o a.out server.o common.o

run-server:
	./server v4 51511
run-client:
	./client 127.0.0.1 51511
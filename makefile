all:
	make compile
clean:
	rm -rf client server client.o a.out server.o common.o

compile:
	gcc -Wall -c ./common.c
	gcc -Wall ./client.c ./common.o -o client
	gcc -Wall ./server.c ./common.o -o server

run-serverv4:
	./server v4 51511

run-clientv4:
	./client 127.0.0.1 51511

run-serverv6:
	./server v6 51511

run-clientv6:
	./client ::1 51511

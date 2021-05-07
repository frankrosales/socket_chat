make:
	gcc -pthread client.c terminal.o -lncurses -o client
	./client

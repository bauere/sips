#sips: a simple ips patcher

CC=gcc
CFLAGS=-std=c99 -pedantic

sips:   sips.o	
	$(CC) -o sips sips.o

sips.o: sips.c
	$(CC) $(CFLAGS) -c sips.c

clean : 
	rm sips sips.o

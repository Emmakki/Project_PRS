all: serveur1

serveur1: serveur1.o
	gcc -Wall serveur1.o -o serveur1

serveur1.o: serveur1.c
	gcc -Wall serveur1.c -c -o serveur1.o

clean:
	rm -f serveur1 *.o
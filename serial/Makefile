HEADERS = serial.h
FLAGS = -Wall -pg 

main: main.o serial.o
	$(CC) $(FLAGS) -o "$@" main.o serial.o

main.o: main.c $(HEADERS)
	$(CC) -c main.c serial.c


serial.o: serial.c $(HEADERS)
	$(CC) -c serial.c 

clean:
	-rm -f serial.o
	-rm -f main.o
	-rm -f main

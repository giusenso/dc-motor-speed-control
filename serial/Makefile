HEADERS = serial.h
FLAGS = -Wall -pg 

main: main.o gui.o serial.o
	$(CC) $(FLAGS) -o "$@" main.o gui.o serial.o -lncurses -lpthread

main.o: main.c $(HEADERS)
	$(CC) -c main.c gui.c serial.c

gui.o: gui.c $(HEADERS)
	$(CC) -c  gui.c serial.c

serial.o: serial.c $(HEADERS)
	$(CC) -c serial.c 

clean:
	-rm -f serial.o
	-rm -f main.o
	-rm -f gui.o
	-rm -f main

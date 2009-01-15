mud: server.o main.o log.o
	gcc server.o main.o log.o -o mud -O2 -W -Wall -Werror

server.o: server.c server.h
	gcc server.c -c

main.o: main.c
	gcc main.c -c

log.o: log.c log.h
	gcc log.c -c

clean:
	rm *.o mud


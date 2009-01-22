CFLAGS:=-O2 -W -Wall -g
#CFLAGS+=-Werror

%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $(filter %.c,$^)

% : %.o
	$(CC) $(LDFLAGS) $(TARGET_ARCH) -o $@ $(filter %.o,$^)

mud: server.o main.o log.o
	$(CC) $(LDFLAGS) $(TARGET_ARCH) -o $@ $(filter %.o,$^)

server.o: server.c server.h

main.o: main.c

log.o: log.c log.h

clean:
	$(RM) *.o mud

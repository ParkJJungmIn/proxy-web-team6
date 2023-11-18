CC = gcc
CFLAGS = -g -Wall
LIB = -lpthread
echoclient: echoclient.c csapp.c csapp.h
	$(CC) $(CFLAGS) -o clienti echoclient.c csapp.c csapp.h $(LIB)
echoserveri: echoserveri.c echo.c csapp.c csapp.h
	$(CC) $(CFLAGS) -o serveri echoserveri.c echo.c csapp.c csapp.h $(LIB)
clean:
	rm -f *.o